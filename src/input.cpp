#include "input_bridge.hpp"
#include <algorithm>
#include <climits>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include <Winamp/in2.h>

// wa_ipc.h uses _MSC_VER to decide whether Windows has already defined intptr_t.
#ifndef _MSC_VER
#define _MSC_VER 1900
#define TS_RESTORE_COMPILER
#endif
#include <Winamp/wa_ipc.h>
#ifdef TS_RESTORE_COMPILER
#undef _MSC_VER
#endif

namespace {
using tangerine::InputRequest;
In_Module input{};
HANDLE lease = INVALID_HANDLE_VALUE;
std::mutex nameMutex;
std::wstring current;
int duration = -1;
In_Module* nativePlayer = nullptr;
Out_Module* previousOutput = nullptr;
std::wstring nativeSource;
std::string ansiSource;

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
    auto address = reinterpret_cast<void*>(GetProcAddress(library, "tangerineInputQueryV2"));
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
    if (nativePlayer) {
        nativePlayer->Stop();
        nativePlayer->outMod = previousOutput;
        nativePlayer = nullptr;
    }
    SecureZeroMemory(nativeSource.data(), nativeSource.size() * sizeof(wchar_t));
    nativeSource.clear();
    SecureZeroMemory(ansiSource.data(), ansiSource.size());
    ansiSource.clear();
    if (lease != INVALID_HANDLE_VALUE) { CloseHandle(lease); lease = INVALID_HANDLE_VALUE; }
    std::wstring finished;
    {
        std::scoped_lock lock(nameMutex);
        finished.swap(current);
    }
    query(InputRequest::ended, finished.c_str());
    input.is_seekable = 0;
    duration = -1;
}

// WACUP selects the input module for the real audio source. It drives the
// normal DSP, visualizations, output and EOF handling for this playback.
bool playNative(const wchar_t* source) {
    auto* module = selectInput(source);
    if (!module || !module->Play || !module->Stop) return false;
    nativeSource = source;
    const wchar_t* name = nativeSource.c_str();
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

int play(const wchar_t* filename) {
    stop();
    try {
        if (!filename || !input.outMod) return 1;
        {
            std::scoped_lock lock(nameMutex);
            current = filename;
        }
        std::vector<wchar_t> source(32768);
        tangerine::SourceAttempts attempts;
        getFileInfo(filename, nullptr, &duration);
        for (;;) {
            auto previous = attempts;
            if (!query(InputRequest::source, filename, nullptr, source.data(), source.size(), &lease, &attempts)) break;
            const bool started = playNative(source.data());
            SecureZeroMemory(source.data(), source.size() * sizeof(wchar_t));
            if (started) {
                if (attempts.rolling) query(InputRequest::started, filename, nullptr, nullptr, 0, nullptr, &previous);
                return 0;
            }
            if (lease != INVALID_HANDLE_VALUE) { CloseHandle(lease); lease = INVALID_HANDLE_VALUE; }
            if (previous.triedCache == attempts.triedCache && previous.excluded == attempts.excluded) break;
        }
    } catch (...) {}
    stop();
    return 1;
}

void about(HWND w) {
    MessageBoxW(w, L"Tangerine Stream input by COOLTRAINER. Configure servers in the Tangerine Stream Media Library view.",
                L"Tangerine Stream", MB_OK);
}
}

extern "C" __declspec(dllexport) int winampGetExtendedFileInfoW(
    const wchar_t* filename, const char* field, wchar_t* output, int capacity) {
    wchar_t tag[80]{};
    if (!field || !MultiByteToWideChar(CP_ACP, 0, field, -1, tag, 80)) return 0;
    return metadata(filename, tag, output, capacity);
}

extern "C" __declspec(dllexport) In_Module* winampGetInModule2() {
    static const bool initialized = [] {
        static char extensions[] = "TSTREAM\0Tangerine Stream track\0";
#ifdef IN_VER_RET
        input.version = IN_VER_RET;
        input.description = reinterpret_cast<char*>(const_cast<wchar_t*>(L"Tangerine Stream Input v0.6.6"));
        input.Init = [] { return 0; };
#else
        input.version = IN_VER;
        input.description = const_cast<char*>("Tangerine Stream Input v0.6.6");
        input.Init = [] {};
#endif
        input.FileExtensions = extensions;
        input.UsesOutputPlug = IN_MODULE_FLAG_USES_OUTPUT_PLUGIN;
        input.Config = input.About = about;
        input.Quit = stop;
        input.GetFileInfo = getFileInfo;
        input.InfoBox = [](const wchar_t*, HWND) { return 1; }; // File info unchanged
        input.IsOurFile = [](const wchar_t* file) { return query(InputRequest::recognize, file); };
        input.Play = play;
        input.Stop = stop;
        input.Pause = [] { if (nativePlayer) nativePlayer->Pause(); };
        input.UnPause = [] { if (nativePlayer) nativePlayer->UnPause(); };
        input.IsPaused = [] { return nativePlayer ? nativePlayer->IsPaused() : 0; };
        input.GetLength = [] {
            return duration >= 0 || !nativePlayer ? duration : nativePlayer->GetLength();
        };
        input.GetOutputTime = [] { return nativePlayer ? nativePlayer->GetOutputTime() : 0; };
        input.SetOutputTime = [](int ms) {
            if (ms >= 0 && nativePlayer && nativePlayer->is_seekable) nativePlayer->SetOutputTime(ms);
        };
        input.SetVolume = [](int volume) { if (nativePlayer) nativePlayer->SetVolume(volume); };
        input.SetPan = [](int pan) { if (nativePlayer) nativePlayer->SetPan(pan); };
        input.EQSet = [](int on, char bands[10], int preamp) {
            if (nativePlayer && nativePlayer->EQSet) nativePlayer->EQSet(on, bands, preamp);
        };
        return true;
    }();
    (void)initialized;
    return &input;
}
