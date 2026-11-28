#pragma once

#include <windows.h>
#include <Winamp/in2.h>
#include <array>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tangerine {

// The host thread opens the output device. The decoder worker feeds it PCM while
// the library downloads the selected representation on its own thread.
class ProgressiveInput {
public:
    enum Message { configure = WM_APP + 2, finished };
    using SelectInput = In_Module* (*)(const wchar_t*);
    ProgressiveInput(In_Module& input, SelectInput select, HWND receiver, UINT_PTR serial, int duration);
    ~ProgressiveInput();
    void start();
    void update(const wchar_t* path, LONGLONG bytes, LONGLONG total);
    void finish(const wchar_t* path);
    void configureOutput(int volume, int pan);
    void pause(bool value);
    void seek(int ms);
    int position();
    bool gainTag(const char* field, wchar_t* value, int capacity);
    bool outputOpen() const { return opened_; } // Called on the host thread.
    void stop();

private:
    struct Codec {
        intptr_t (__cdecl *open)(const wchar_t*, int*, int*, int*, int*) = nullptr;
        intptr_t (__cdecl *read)(intptr_t, char*, size_t, int*) = nullptr;
        void (__cdecl *close)(intptr_t) = nullptr;
        int (__cdecl *seek)(intptr_t, int) = nullptr;
        int (__cdecl *tag)(const wchar_t*, const char*, wchar_t*, int) = nullptr;
        explicit Codec(In_Module* module);
        explicit operator bool() const { return open && read && close; }
    };
    struct Format {
        int rate = 0, channels = 0, bits = 0;
        int stride() const { return channels * (bits / 8); }
        bool valid() const;
        bool operator==(const Format& o) const { return rate == o.rate && channels == o.channels && bits == o.bits; }
    };
    struct Stream;
    enum class Result { wait, end, delegate };
    void run();
    Result decode(Stream& stream, uint64_t epoch, bool final, int64_t& frame);
    bool interrupted(uint64_t epoch);
    bool waitOutput(uint64_t epoch, int bytes);
    bool openOutput(const Format& format, uint64_t epoch);
    In_Module& input_;
    Codec flac_, mp3_;
    HWND receiver_;
    UINT_PTR serial_;
    int duration_;
    bool readGain_ = true;
    int preferredBits_;
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable changed_;
    std::wstring path_;
    int64_t bytes_ = 0, total_ = -1;
    uint64_t epoch_ = 0, configureEpoch_ = 0;
    bool final_ = false, stopped_ = false, paused_ = false;
    int seekMs_ = -1, displayedSeek_ = -1, flushMs_ = -1;
    Format source_, output_;
    std::array<std::array<wchar_t, 64>, 4> gainTags_{};
    bool opened_ = false, outputFailed_ = false;
    int killswitch_ = 0; // Extended Read ABI. Set atomically when cancelling a decoder call.
    int64_t writtenFrame_ = 0;
    std::vector<char> history_;
};

}
