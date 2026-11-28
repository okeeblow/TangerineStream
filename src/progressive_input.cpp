#include "progressive_input.hpp"
#include "host_profile.hpp"
#include "sdk_services.hpp"
#include <api/service/waservicefactory.h>
#include <Config/api_config.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstring>
#include <filesystem>

namespace tangerine {
namespace {
using namespace std::chrono_literals;
constexpr int visualFrames = 576;

constexpr const char* gainKeys[] = {"replaygain_track_gain", "replaygain_track_peak", "replaygain_album_gain", "replaygain_album_peak"};
int outputBits(HWND host) {
    const GUID playbackGroup{0xb6cb4a7c, 0xa8d0, 0x4c55, {0x8e, 0x60, 0x9f, 0x7a, 0x7a, 0x23, 0xda, 0x0f}};
    auto* services = reinterpret_cast<api_service*>(SendMessageW(host, WM_WA_IPC, 0, IPC_GET_API_SERVICE));
    auto* factory = reinterpret_cast<UINT_PTR>(services) > 1 ? services->service_getServiceByGuid(AgaveConfigGUID) : nullptr;
    if (!factory) return 16;
    auto* config = static_cast<api_config*>(factory->getInterface());
    if (!config) return 16;
    auto bits = config->GetUnsigned(playbackGroup, L"bits", 16);
    factory->releaseInterface(config);
    return bits == 24 || bits == 32 ? static_cast<int>(bits) : 16;
}

template<typename T> T entry(HMODULE dll, const char* name) {
    return reinterpret_cast<T>(reinterpret_cast<void*>(GetProcAddress(dll, name)));
}
int milliseconds(int64_t frame, int rate) {
    return static_cast<int>(std::min<int64_t>(INT_MAX, frame * 1000 / rate));
}

class FileHandle {
public:
    explicit FileHandle(HANDLE file) : value(file) {}
    ~FileHandle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    FileHandle(const FileHandle&) = delete;
    HANDLE value;
};

// Decoders can conceal a truncated frame if the file grows during a read.
// Present a fixed prefix per decoder handle. Extend it only while closed,
// copying each downloaded byte at most once across all normal underruns.
class Preview {
public:
    ~Preview() { clear(); }
    void clear() {
        if (!path.empty()) DeleteFileW(path.c_str());
        path.clear();
        copied = 0;
    }
    bool extend(const std::wstring& source, int64_t available) {
        auto name = std::filesystem::path(source);
        auto leaf = name.filename().wstring();
        if (leaf.compare(0, 5, L"play-") != 0) return false;
        leaf.replace(0, 5, L"read-");
        name.replace_filename(leaf);
        if (path != name.native()) { clear(); path = name.native(); }
        // Bound work before returning to the decoder, even on a fast LAN.
        auto end = std::min<int64_t>(available, copied + 4 * 1024 * 1024);
        if (copied == end) return true;
        FileHandle input(CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
        FileHandle output(CreateFileW(path.c_str(), FILE_APPEND_DATA, 0, nullptr, copied ? OPEN_EXISTING : CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL, nullptr));
        LARGE_INTEGER position;
        position.QuadPart = copied;
        if (input.value == INVALID_HANDLE_VALUE || output.value == INVALID_HANDLE_VALUE ||
            !SetFilePointerEx(input.value, position, nullptr, FILE_BEGIN)) return false;
        char buffer[64 * 1024];
        while (copied < end) {
            DWORD n = 0, written = 0;
            DWORD request = static_cast<DWORD>(std::min<int64_t>(sizeof(buffer), end - copied));
            if (!ReadFile(input.value, buffer, request, &n, nullptr) || !n ||
                !WriteFile(output.value, buffer, n, &written, nullptr) || written != n) return false;
            copied += n;
        }
        return true;
    }
    std::wstring path;
    int64_t copied = 0;
};
}

ProgressiveInput::Codec::Codec(In_Module* module) {
    if (!module || !module->hDllInstance) return;
    auto dll = module->hDllInstance;
    open = entry<decltype(open)>(dll, "winampGetExtendedRead_openW");
    read = entry<decltype(read)>(dll, "winampGetExtendedRead_getData");
    close = entry<decltype(close)>(dll, "winampGetExtendedRead_close");
    seek = entry<decltype(seek)>(dll, "winampGetExtendedRead_setTime");
    tag = entry<decltype(tag)>(dll, "winampGetExtendedFileInfoW");
}

bool ProgressiveInput::Format::valid() const {
    return rate > 0 && rate <= 768000 && channels > 0 && channels <= 32 &&
           (bits == 16 || bits == 24 || bits == 32);
}

struct ProgressiveInput::Stream {
    const Codec& codec;
    const std::wstring& path;
    int requestedBits;
    int& killswitch;
    Format format;
    intptr_t handle = 0;
    bool open() {
        if (handle) codec.close(handle);
        format = {};
        format.bits = requestedBits;
        int size = -1;
        handle = codec.open(path.c_str(), &size, &format.bits, &format.channels, &format.rate);
        return handle && format.valid();
    }
    ~Stream() { if (handle) codec.close(handle); }
    int read(char* dest, int bytes) {
        auto got = codec.read(handle, dest, bytes, &killswitch);
        return got >= 0 && got <= bytes && got % format.stride() == 0 ? static_cast<int>(got) : -1;
    }
};

ProgressiveInput::ProgressiveInput(In_Module& input, SelectInput select, HWND receiver, UINT_PTR serial, int duration)
    : input_(input), flac_(select(L"Tangerine Stream.flac")), mp3_(select(L"Tangerine Stream.mp3")),
      receiver_(receiver), serial_(serial), duration_(duration), preferredBits_(outputBits(input.hMainWindow)) {}

ProgressiveInput::~ProgressiveInput() { stop(); }
void ProgressiveInput::start() { worker_ = std::thread(&ProgressiveInput::run, this); }

void ProgressiveInput::update(const wchar_t* path, LONGLONG bytes, LONGLONG total) {
    std::scoped_lock lock(mutex_);
    if (path) {
        if (path_.empty()) { path_ = path; ++epoch_; }
        bytes_ = bytes;
        total_ = total;
        changed_.notify_one();
    } else {
        path_.clear();
        bytes_ = 0;
        ++epoch_;
        InterlockedExchange(reinterpret_cast<volatile LONG*>(&killswitch_), 1);
        changed_.notify_all();
    }
}

void ProgressiveInput::finish(const wchar_t* path) {
    std::scoped_lock lock(mutex_);
    path_ = path ? path : L"";
    final_ = true;
    ++epoch_;
    changed_.notify_all();
}

bool ProgressiveInput::interrupted(uint64_t epoch) {
    std::scoped_lock lock(mutex_);
    return stopped_ || epoch_ != epoch;
}

bool ProgressiveInput::openOutput(const Format& format, uint64_t epoch) {
    std::unique_lock lock(mutex_);
    if (!opened_ && !outputFailed_ && !stopped_ && epoch_ == epoch) {
        source_ = output_ = format;
        output_.bits = std::min(format.bits, preferredBits_);
        configureEpoch_ = epoch;
        if (!PostMessageW(receiver_, configure, serial_, 0)) return false;
        changed_.wait(lock, [&] { return stopped_ || epoch != epoch_ || opened_ || outputFailed_; });
    }
    return opened_ && source_ == format;
}

void ProgressiveInput::configureOutput(int volume, int pan) {
    Format format;
    int bitrate;
    {
        std::scoped_lock lock(mutex_);
        if (stopped_ || opened_ || outputFailed_ || !output_.valid() || epoch_ != configureEpoch_) return;
        format = output_;
        bitrate = duration_ > 0 && total_ > 0 ? static_cast<int>(std::min<double>(INT_MAX, 8.0 * total_ / duration_)) : 0;
    }
    int latency = input_.outMod->Open(format.rate, format.channels, format.bits, -1, -1);
    if (latency >= 0) {
        if (input_.SAVSAInit) input_.SAVSAInit(latency, format.rate);
        if (input_.VSASetInfo) input_.VSASetInfo(format.rate, format.channels);
        if (input_.SetInfo) input_.SetInfo(bitrate, format.rate / 1000, format.channels, 1);
        if (volume >= 0) input_.outMod->SetVolume(volume);
        input_.outMod->SetPan(pan);
        if (paused_) input_.outMod->Pause(1);
    }
    std::scoped_lock lock(mutex_);
    opened_ = latency >= 0;
    outputFailed_ = !opened_;
    changed_.notify_all();
}

bool ProgressiveInput::waitOutput(uint64_t epoch, int bytes) {
    std::unique_lock lock(mutex_);
    while (!stopped_ && epoch_ == epoch) {
        if (flushMs_ >= 0) {
            const int ms = flushMs_;
            flushMs_ = -1;
            lock.unlock();
            input_.outMod->Flush(ms);
            lock.lock();
            continue;
        }
        if (!paused_) {
            lock.unlock();
            int room = input_.outMod->CanWrite();
            lock.lock();
            if (room >= bytes && !paused_ && !stopped_ && epoch_ == epoch) return true;
        }
        changed_.wait_for(lock, 5ms);
    }
    return false;
}

ProgressiveInput::Result ProgressiveInput::decode(Stream& stream, uint64_t epoch, bool final, int64_t& frame) {
    if (!stream.open()) return final ? Result::delegate : Result::wait;
    const Format format = stream.format;
    const int stride = format.stride();
    const int outBits = std::min(format.bits, preferredBits_), outStride = format.channels * (outBits / 8);
    // Out_Module::Write accepts at most 8192 bytes, including DSP expansion.
    const int block = std::min(visualFrames, 8192 / (2 * outStride));
    const int blockBytes = block * stride;
    std::vector<char> buffer(64 * 1024);
    {
        std::scoped_lock lock(mutex_);
        if (opened_ && !(format == source_)) return Result::delegate;
        if (seekMs_ >= 0) {
            frame = int64_t(seekMs_) * format.rate / 1000;
            seekMs_ = -1;
            history_.clear();
        }
    }
    // A codec's millisecond seek can land on a packet boundary. Verify the last
    // submitted PCM block at the join. A nonseekable or imprecise decoder can
    // always recover exactly by decoding the same frame count from the start.
    auto discard = [&](int64_t frames) {
        while (frames > 0 && !interrupted(epoch)) {
            int n = static_cast<int>(std::min<int64_t>(frames, buffer.size() / stride)) * stride;
            int got = stream.read(buffer.data(), n);
            if (got <= 0) return false;
            frames -= got / stride;
        }
        return frames == 0;
    };
    auto matchHistory = [&] {
        size_t off = 0;
        while (off < history_.size() && !interrupted(epoch)) {
            int got = stream.read(buffer.data(), static_cast<int>(history_.size() - off));
            if (got <= 0 || std::memcmp(buffer.data(), history_.data() + off, got)) return false;
            off += got;
        }
        return off == history_.size();
    };
    int64_t checkpoint = frame - static_cast<int64_t>(history_.size() / stride);
    int seconds = static_cast<int>(checkpoint / format.rate);
    bool aligned = false;
    if (seconds > 0 && &stream.codec == &flac_ && !history_.empty() && stream.codec.seek) {
        if (stream.codec.seek(stream.handle, seconds * 1000))
            aligned = discard(checkpoint - int64_t(seconds) * format.rate) && matchHistory();
        if (!aligned && (!stream.open() || !(stream.format == format))) return final ? Result::delegate : Result::wait;
    }
    if (!aligned) {
        if (!discard(checkpoint)) return final ? Result::end : Result::wait;
        if (!matchHistory()) return final ? Result::delegate : Result::wait;
    }
    std::vector<char> ahead, pcm(2 * block * outStride);
    // A decoder may conceal a truncated packet. Hold its tail until the next
    // packets arrive, then deliver the last samples from the completed file.
    const int lookahead = final ? 0 : (&stream.codec == &mp3_ ? 4 * block : format.rate / 8) * stride;
    ahead.resize(8 * blockBytes + lookahead);
    int used = 0, offset = 0, got = 1;
    while (!interrupted(epoch)) {
        if (used - offset < blockBytes + lookahead && got > 0) {
            used -= offset;
            std::memmove(ahead.data(), ahead.data() + offset, used);
            offset = 0;
            while (used < static_cast<int>(ahead.size())) {
                got = stream.read(ahead.data() + used, static_cast<int>(ahead.size()) - used);
                if (got <= 0) break; // Reopen after growth. Never read through a latched EOF.
                used += got;
            }
        }
        int count = std::min(blockBytes, used - offset - lookahead);
        if (count <= 0) return !final ? Result::wait : got < 0 ? Result::delegate : Result::end;
        if (readGain_ && stream.codec.tag) {
            decltype(gainTags_) tags{};
            for (size_t i = 0; i < tags.size(); ++i)
                stream.codec.tag(stream.path.c_str(), gainKeys[i], tags[i].data(), static_cast<int>(tags[i].size()));
            std::scoped_lock lock(mutex_);
            gainTags_ = tags;
            readGain_ = false;
        }
        if (!openOutput(format, epoch)) return interrupted(epoch) ? Result::wait : Result::delegate;
        int frames = count / stride;
        bool dsp = input_.dsp_isactive && input_.dsp_isactive();
        if (!waitOutput(epoch, frames * outStride * (dsp ? 2 : 1))) break;
        auto* raw = ahead.data() + offset;
        if (format.bits == outBits) std::memcpy(pcm.data(), raw, count);
        else {
            // Packed little-endian integer PCM. Keep the most significant bytes
            // of each sample for the bit depth configured in the player.
            const int inWidth = format.bits / 8, outWidth = outBits / 8;
            for (int i = 0; i < frames * format.channels; ++i)
                std::memcpy(pcm.data() + i*outWidth, raw + (i+1)*inWidth - outWidth, outWidth);
        }
        const int ms = milliseconds(frame, format.rate);
        if (frames == visualFrames) {
            if (input_.SAAddPCMData) input_.SAAddPCMData(pcm.data(), format.channels, outBits, ms);
            if (input_.VSAAddPCMData) input_.VSAAddPCMData(pcm.data(), format.channels, outBits, ms);
        }
        int samples = dsp ? input_.dsp_dosamples(reinterpret_cast<short*>(pcm.data()), frames, outBits, format.channels, format.rate) : frames;
        if (samples < 0 || samples > frames * 2) return Result::delegate;
        while (samples && input_.outMod->Write(pcm.data(), samples * outStride)) {
            if (!waitOutput(epoch, samples * outStride)) return Result::wait;
        }
        history_.assign(raw, raw + count);
        frame += frames;
        offset += count;
        std::scoped_lock lock(mutex_);
        writtenFrame_ = frame;
        if (seekMs_ < 0) displayedSeek_ = -1;
    }
    return Result::wait;
}

void ProgressiveInput::run() {
    Preview preview;
    Result result = Result::wait;
    int64_t frame = 0, nextBytes = 0;
    uint64_t previous = 0;
    bool draining = false;
    try {
        for (;;) {
            std::unique_lock lock(mutex_);
            changed_.wait(lock, [&] {
                return stopped_ || final_ || (opened_ && flushMs_ >= 0) ||
                    (result != Result::delegate && !path_.empty() && (epoch_ != previous || bytes_ >= nextBytes));
            });
            if (stopped_) return;
            if (opened_ && flushMs_ >= 0) {
                int ms = flushMs_;
                flushMs_ = -1;
                lock.unlock();
                input_.outMod->Flush(ms);
                continue;
            }
            if (draining && previous == epoch_) {
                if (!opened_) break;
                lock.unlock();
                bool busy = input_.outMod->IsPlaying() != 0;
                lock.lock();
                if (!busy && previous == epoch_) break;
                changed_.wait_for(lock, 10ms);
                continue;
            }
            draining = false;
            if (final_ && (path_.empty() || result == Result::delegate)) {
                result = path_.empty() ? Result::end : Result::delegate;
                draining = true;
                previous = epoch_;
                continue;
            }
            std::wstring path = path_;
            const auto suffix = std::wcsrchr(path.c_str(), L'.');
            auto* codec = suffix && !_wcsicmp(suffix, L".flac") ? &flac_ : suffix && !_wcsicmp(suffix, L".mp3") ? &mp3_ : nullptr;
            if (!codec || !*codec) { result = Result::delegate; continue; }
            const bool complete = final_;
            const int64_t available = bytes_;
            previous = epoch_;
            const int64_t growth = total_ > 0 && duration_ > 0 ? total_ / duration_ * 250 : 64 * 1024;
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&killswitch_), 0);
            lock.unlock();
            if (complete) preview.clear();
            if (complete || preview.extend(path, available)) {
                Stream stream{*codec, complete ? path : preview.path, preferredBits_, killswitch_, {}};
                result = decode(stream, previous, complete, frame);
            } else result = Result::delegate;
            if (result == Result::delegate) preview.clear();
            lock.lock();
            if (stopped_) return;
            nextBytes = preview.copied + std::clamp<int64_t>(growth, 16 * 1024, 256 * 1024);
            if (previous != epoch_) { result = Result::wait; continue; }
            draining = complete && result != Result::wait;
        }
    } catch (...) {
        std::unique_lock lock(mutex_);
        changed_.wait(lock, [&] { return stopped_ || final_; });
        if (stopped_) return;
        result = Result::delegate;
    }
    PostMessageW(receiver_, finished, serial_, result == Result::delegate);
}

void ProgressiveInput::pause(bool value) {
    bool active;
    {
        std::scoped_lock lock(mutex_);
        paused_ = value;
        active = opened_;
        changed_.notify_all();
    }
    if (active) input_.outMod->Pause(value);
}

void ProgressiveInput::seek(int ms) {
    std::scoped_lock lock(mutex_);
    seekMs_ = displayedSeek_ = flushMs_ = ms;
    ++epoch_;
    InterlockedExchange(reinterpret_cast<volatile LONG*>(&killswitch_), 1);
    changed_.notify_all();
}

int ProgressiveInput::position() {
    std::unique_lock lock(mutex_);
    if (displayedSeek_ >= 0) return displayedSeek_;
    if (!opened_) return 0;
    int ms = milliseconds(writtenFrame_, output_.rate);
    lock.unlock();
    return std::max(0, ms - std::max(0, input_.outMod->GetWrittenTime() - input_.outMod->GetOutputTime()));
}

bool ProgressiveInput::gainTag(const char* field, wchar_t* value, int capacity) {
    for (size_t i = 0; i < gainTags_.size(); ++i) {
        if (_stricmp(field, gainKeys[i])) continue;
        std::scoped_lock lock(mutex_);
        if (!gainTags_[i].front()) return false;
        lstrcpynW(value, gainTags_[i].data(), capacity);
        return true;
    }
    return false;
}

void ProgressiveInput::stop() {
    {
        std::scoped_lock lock(mutex_);
        stopped_ = true;
        InterlockedExchange(reinterpret_cast<volatile LONG*>(&killswitch_), 1);
        changed_.notify_all();
    }
    if (worker_.joinable()) worker_.join();
    if (opened_) {
        input_.outMod->Close();
        if (input_.SAVSADeInit) input_.SAVSADeInit();
        opened_ = false;
    }
}
}
