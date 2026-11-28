#include "input_bridge.hpp"
#include "progressive_input.hpp"
#include "tangerine_product.h"
#include <algorithm>
#include <array>
#include <climits>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <Winamp/in2.h>

#include "host_profile.hpp"

namespace {
using tangerine::InputRequest;
In_Module input{};
std::mutex nameMutex;
std::wstring current;
std::weak_ptr<tangerine::ProgressiveInput> metadataPlayer;
int duration = -1;
In_Module* nativePlayer = nullptr;
Out_Module* previousOutput = nullptr;
std::string ansiSource;
bool paused = false;
int pendingSeek = -1, volume = -1, pan = 0;
int equalizerEnabled = 0, equalizerPreamp = 0;
std::array<char, 10> equalizerBands{};
bool haveEqualizer = false;

struct AudioFile {
    std::vector<wchar_t> path = std::vector<wchar_t>(32768);
    HANDLE lease = INVALID_HANDLE_VALUE;
    tangerine::SourceAttempts attempts;
    ~AudioFile() {
        if (lease != INVALID_HANDLE_VALUE) CloseHandle(lease);
        if (attempts.temporary && path.front()) DeleteFileW(path.data());
    }
};
std::unique_ptr<AudioFile> playingFile;

struct Download {
    std::wstring filename;
    unsigned excluded;
    UINT_PTR serial;
    bool cached = false;
    HANDLE cancel = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::shared_ptr<tangerine::ProgressiveInput> player;
    ~Download() { if (cancel) CloseHandle(cancel); }
};
std::mutex downloadMutex;
std::condition_variable downloadReady;
std::shared_ptr<Download> loading, queued;
std::shared_ptr<tangerine::ProgressiveInput> progressive;
std::unique_ptr<AudioFile> completed;
std::thread downloader;
bool shuttingDown = false;
UINT_PTR serial = 0;
HWND receiver = nullptr;
constexpr wchar_t receiverClass[] = L"TangerineStream.InputPlayback";

In_Module* selectInput(const wchar_t* source) {
    auto* module = reinterpret_cast<In_Module*>(SendMessageW(input.hMainWindow, WM_WA_IPC,
        reinterpret_cast<WPARAM>(source), IPC_CANPLAY));
    return reinterpret_cast<UINT_PTR>(module) > 1 && module != &input ? module : nullptr;
}

int query(InputRequest op, const wchar_t* filename, const wchar_t* field = nullptr,
          wchar_t* value = nullptr, size_t capacity = 0, HANDLE* file = nullptr, tangerine::SourceAttempts* attempts = nullptr) {
    if (!filename || !*filename) return 0;
    std::wstring_view name(filename);
    bool marker = name.size() > 8 && !_wcsicmp(filename + name.size() - 8, L".tstream");
    if (!marker && name.substr(0, 17) != L"http://127.0.0.1:") return 0;
    HMODULE library = nullptr;
    if (!GetModuleHandleExW(0, L"ml_tangerinestream.dll", &library)) return 0;
    auto address = reinterpret_cast<void*>(GetProcAddress(library, "tangerineInputQueryV4"));
    auto fn = reinterpret_cast<tangerine::InputQuery>(address);
    int result = fn ? fn(op, filename, field, value, capacity, file, attempts) : 0;
    FreeLibrary(library);
    return result;
}

int metadata(const wchar_t* filename, const wchar_t* field, wchar_t* value, int capacity) {
    if (!value || capacity <= 0) return 0;
    if (filename && *filename) return query(InputRequest::metadata, filename, field, value, capacity);
    std::scoped_lock lock(nameMutex);
    return query(InputRequest::metadata, current.c_str(), field, value, capacity);
}

void getFileInfo(const wchar_t* filename, wchar_t* title, int* length) {
    if (title) {
        title[0] = 0;
        metadata(filename, L"streamtitle", title, GETFILEINFO_TITLE_LENGTH);
    }
    if (length) {
        wchar_t ms[32]{};
        *length = metadata(filename, L"length", ms, 32)
            ? static_cast<int>(std::min<long long>(_wtoi64(ms), INT_MAX)) : -1;
    }
}

void stop() {
    std::unique_ptr<AudioFile> downloaded;
    {
        std::scoped_lock lock(downloadMutex);
        if (loading) SetEvent(loading->cancel);
        loading.reset();
        queued.reset();
        downloaded = std::move(completed);
    }
    if (progressive) { progressive->stop(); progressive.reset(); }
    if (nativePlayer) {
        nativePlayer->Stop();
        nativePlayer->outMod = previousOutput;
        nativePlayer = nullptr;
    }
    ansiSource.clear();
    playingFile.reset();
    std::wstring finished;
    {
        std::scoped_lock lock(nameMutex);
        finished.swap(current);
        metadataPlayer.reset();
    }
    query(InputRequest::ended, finished.c_str());
    input.is_seekable = 0;
    duration = -1;
    pendingSeek = -1;
    paused = false;
}

// WACUP selects the input module for the real audio source. It drives the
// normal DSP, visualizations, output and EOF handling for this playback.
bool playNative(const wchar_t* source) {
    auto* module = selectInput(source);
    if (!module || !module->Play || !module->Stop) return false;
    const wchar_t* name = source; // playingFile owns the path until the decoder stops.
    if (!(module->version & IN_UNICODE)) {
        int n = WideCharToMultiByte(CP_ACP, 0, name, -1, nullptr, 0, nullptr, nullptr);
        if (n <= 0) return false;
        ansiSource.resize(n);
        WideCharToMultiByte(CP_ACP, 0, name, -1, ansiSource.data(), n, nullptr, nullptr);
        name = reinterpret_cast<const wchar_t*>(ansiSource.data());
    }
    previousOutput = module->outMod;
    module->outMod = input.outMod;
    nativePlayer = module;
    if (!module->Play(name)) {
        input.is_seekable = module->is_seekable;
        return true;
    }
    module->outMod = previousOutput;
    nativePlayer = nullptr;
    return false;
}

void downloadLoop() {
    for (;;) {
        std::shared_ptr<Download> job;
        {
            std::unique_lock lock(downloadMutex);
            downloadReady.wait(lock, [] { return shuttingDown || queued; });
            if (shuttingDown) return;
            job = std::move(queued);
        }
        std::unique_ptr<AudioFile> file;
        try {
            file = std::make_unique<AudioFile>();
            file->attempts.excluded = job->excluded;
            file->attempts.cancel = job->cancel;
            file->attempts.progress = {job->player.get(), [](void* context, const wchar_t* path, LONGLONG bytes, LONGLONG total) {
                static_cast<tangerine::ProgressiveInput*>(context)->update(path, bytes, total);
            }};
            if (query(InputRequest::fetch, job->filename.c_str(), nullptr, file->path.data(), file->path.size(),
                      &file->lease, &file->attempts) != tangerine::fileReady) file.reset();
        } catch (...) { file.reset(); }
        std::scoped_lock lock(downloadMutex);
        if (loading == job) {
            completed = std::move(file);
            job->player->finish(completed ? completed->path.data() : nullptr);
        }
    }
}

bool startFile(std::unique_ptr<AudioFile>& file) {
    if (!playNative(file->path.data())) return false;
    playingFile = std::move(file);
    if (volume >= 0 && nativePlayer->SetVolume) nativePlayer->SetVolume(volume);
    if (nativePlayer->SetPan) nativePlayer->SetPan(pan);
    if (haveEqualizer && nativePlayer->EQSet) nativePlayer->EQSet(equalizerEnabled, equalizerBands.data(), equalizerPreamp);
    if (pendingSeek >= 0 && input.is_seekable && nativePlayer->SetOutputTime) nativePlayer->SetOutputTime(pendingSeek);
    pendingSeek = -1;
    if (paused && nativePlayer->Pause) nativePlayer->Pause();
    return true;
}

bool startSource(const std::wstring& filename, unsigned excluded, std::unique_ptr<AudioFile> cached = {});
LRESULT CALLBACK receiverProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == tangerine::ProgressiveInput::configure) {
        if (progressive && w == serial) progressive->configureOutput(volume, pan);
        return 0;
    }
    if (msg != tangerine::ProgressiveInput::finished) return DefWindowProcW(hwnd, msg, w, l);
    std::unique_ptr<AudioFile> file;
    std::shared_ptr<Download> job;
    {
        std::scoped_lock lock(downloadMutex);
        if (!loading || loading->serial != w) return 0;
        job = std::move(loading);
        file = std::move(completed);
    }
    try {
        if (l && file) {
            if (progressive->outputOpen()) pendingSeek = progressive->position();
            progressive->stop();
            progressive.reset();
            {
                std::scoped_lock lock(nameMutex);
                metadataPlayer.reset();
            }
            if (startFile(file)) return 0;
            if ((job->cached || file->attempts.excluded != job->excluded) && startSource(job->filename, file->attempts.excluded))
                return 0;
        }
    } catch (...) {}
    // Already on WACUP's thread. Advance this item before a later Play can replace it.
    SendMessageW(input.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
    return 0;
}

bool startSource(const std::wstring& filename, unsigned excluded, std::unique_ptr<AudioFile> cached) {
    if (!receiver) {
        WNDCLASSW cls{};
        cls.lpfnWndProc = receiverProc;
        cls.hInstance = input.hDllInstance;
        cls.lpszClassName = receiverClass;
        if (!RegisterClassW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        receiver = CreateWindowExW(0, receiverClass, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, cls.hInstance, nullptr);
        if (!receiver) return false;
    }
    auto job = std::make_shared<Download>();
    if (!job->cancel) return false;
    job->filename = filename;
    job->excluded = excluded;
    job->serial = ++serial;
    job->cached = bool(cached);
    job->player = std::make_shared<tangerine::ProgressiveInput>(input, selectInput, receiver, serial, duration);
    job->player->pause(paused);
    if (pendingSeek >= 0) job->player->seek(pendingSeek);
    job->player->start();
    progressive = job->player;
    {
        std::scoped_lock lock(nameMutex);
        metadataPlayer = progressive;
    }
    {
        std::scoped_lock lock(downloadMutex);
        loading = job;
        if (cached) {
            completed = std::move(cached);
            job->player->finish(completed->path.data());
        } else {
            if (!downloader.joinable()) downloader = std::thread(downloadLoop);
            queued = job;
            downloadReady.notify_one();
        }
    }
    input.is_seekable = 1; // Remember a seek made while the file is buffering.
    return true;
}

int play(const wchar_t* filename) {
    stop();
    try {
        if (!filename || !input.outMod) return 1;
        {
            std::scoped_lock lock(nameMutex);
            current = filename;
        }
        getFileInfo(filename, nullptr, &duration);
        auto file = std::make_unique<AudioFile>();
        int status = query(InputRequest::begin, filename, nullptr, file->path.data(), file->path.size(),
                           &file->lease, &file->attempts);
        const unsigned excluded = file->attempts.excluded;
        if (status == tangerine::fileReady && startSource(filename, excluded, std::move(file))) return 0;
        if (status == tangerine::downloadNeeded && startSource(filename, excluded)) return 0;
    } catch (...) {}
    stop();
    return 1;
}

void quit() {
    stop();
    {
        std::scoped_lock lock(downloadMutex);
        shuttingDown = true;
    }
    downloadReady.notify_one();
    if (downloader.joinable()) downloader.join();
    if (receiver) { DestroyWindow(receiver); receiver = nullptr; }
    UnregisterClassW(receiverClass, input.hDllInstance);
    shuttingDown = false;
}

void about(HWND w) {
    MessageBoxW(w, TS_PRODUCT_NAME_W L" input by COOLTRAINER. Configure servers in the "
                TS_PRODUCT_NAME_W L" Media Library view.", TS_PRODUCT_NAME_W, MB_OK);
}

void registerDescription() noexcept {
    try {
        tangerine::setDefaultPluginDescription(tangerine::settingsDirectory(input.hMainWindow),
            L"input", L"in_tangerinestream.dll", L"Plays tracks from your server or cache with WACUP's audio decoders.");
    } catch (...) {} // The Preferences subtitle is optional during input initialization.
}
}

extern "C" __declspec(dllexport) int winampGetExtendedFileInfoW(
    const wchar_t* filename, const char* field, wchar_t* output, int capacity) {
    if (field && output && capacity > 0 && !_strnicmp(field, "replaygain_", 11)) {
        std::shared_ptr<tangerine::ProgressiveInput> player;
        {
            std::scoped_lock lock(nameMutex);
            if (!filename || !*filename || !_wcsicmp(filename, current.c_str())) player = metadataPlayer.lock();
        }
        if (player && player->gainTag(field, output, capacity)) return 1;
    }
    wchar_t tag[80]{};
    if (!field || !MultiByteToWideChar(CP_ACP, 0, field, -1, tag, 80)) return 0;
    return metadata(filename, tag, output, capacity);
}

extern "C" __declspec(dllexport) In_Module* winampGetInModule2() {
    static const bool initialized = [] {
        static char extensions[] = "TSTREAM\0" TS_PRODUCT_NAME " track\0";
#ifdef IN_VER_RET
        static wchar_t description[] = TS_PRODUCT_NAME_W L" Input v" TS_VERSION_STRING;
        input.version = IN_VER_RET;
        input.description = reinterpret_cast<char*>(description);
        input.Init = [] { registerDescription(); return 0; };
#else
        static char description[] = TS_PRODUCT_NAME " Input v" TS_VERSION_STRING;
        input.version = IN_VER;
        input.description = description;
        input.Init = registerDescription;
#endif
        input.FileExtensions = extensions;
        input.UsesOutputPlug = IN_MODULE_FLAG_USES_OUTPUT_PLUGIN;
        input.Config = input.About = about;
        input.Quit = quit;
        input.GetFileInfo = getFileInfo;
        input.InfoBox = [](const wchar_t*, HWND) { return 1; }; // File info unchanged
        input.IsOurFile = [](const wchar_t* file) { return query(InputRequest::recognize, file); };
        input.Play = play;
        input.Stop = stop;
        input.Pause = [] { paused = true; if (nativePlayer) nativePlayer->Pause(); else if (progressive) progressive->pause(true); };
        input.UnPause = [] { paused = false; if (nativePlayer) nativePlayer->UnPause(); else if (progressive) progressive->pause(false); };
        input.IsPaused = [] { return nativePlayer ? nativePlayer->IsPaused() : int(paused); };
        input.GetLength = [] {
            return duration >= 0 || !nativePlayer ? duration : nativePlayer->GetLength();
        };
        input.GetOutputTime = [] { return nativePlayer ? nativePlayer->GetOutputTime() : progressive ? progressive->position() : std::max(0, pendingSeek); };
        input.SetOutputTime = [](int ms) {
            if (ms >= 0 && nativePlayer && nativePlayer->is_seekable) nativePlayer->SetOutputTime(ms);
            else if (ms >= 0 && !nativePlayer) { pendingSeek = ms; if (progressive) progressive->seek(ms); }
        };
        input.SetVolume = [](int level) {
            if (level >= 0) volume = level;
            if (nativePlayer) nativePlayer->SetVolume(level);
            else if (progressive && progressive->outputOpen()) input.outMod->SetVolume(level);
        };
        input.SetPan = [](int level) {
            pan = level;
            if (nativePlayer) nativePlayer->SetPan(level);
            else if (progressive && progressive->outputOpen()) input.outMod->SetPan(level);
        };
        input.EQSet = [](int on, char bands[10], int preamp) {
            if (bands) {
                equalizerEnabled = on;
                equalizerPreamp = preamp;
                std::copy_n(bands, equalizerBands.size(), equalizerBands.begin());
                haveEqualizer = true;
            }
            if (nativePlayer && nativePlayer->EQSet) nativePlayer->EQSet(on, bands, preamp);
        };
        return true;
    }();
    (void)initialized;
    return &input;
}
