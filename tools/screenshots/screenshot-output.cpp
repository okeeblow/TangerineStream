#include <windows.h>
#include <Winamp/out.h>

#include <algorithm>
#include <chrono>
#include <mutex>

namespace {
using Clock = std::chrono::steady_clock;
std::mutex clockMutex;
Clock::time_point clockStart;
int bytesPerSecond = 0;
int positionAtStartMs = 0;
long long writtenBytes = 0;
bool paused = false;

int writtenTimeMs() {
    return bytesPerSecond ? static_cast<int>(1000 * writtenBytes / bytesPerSecond) : 0;
}

int playedTimeMs() {
    auto elapsedMs = paused ? 0 : std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - clockStart).count();
    return static_cast<int>(std::min<long long>(writtenTimeMs(), positionAtStartMs + elapsedMs));
}

void resetPosition(int positionMs) {
    positionAtStartMs = positionMs;
    writtenBytes = static_cast<long long>(positionMs) * bytesPerSecond / 1000;
    clockStart = Clock::now();
}

int freeBufferBytes() {
    if (!bytesPerSecond) return 0;
    // Leave enough space for a decoder block even on low-rate mono streams.
    const int capacityBytes = std::max(65536, bytesPerSecond);
    const auto playedBytes = static_cast<long long>(playedTimeMs()) * bytesPerSecond / 1000;
    return std::max(0, static_cast<int>(capacityBytes - (writtenBytes - playedBytes)));
}

int open(int sampleRate, int channels, int bitsPerSample, int, int) {
    std::lock_guard lock(clockMutex);
    if (sampleRate <= 0 || channels <= 0 || bitsPerSample <= 0) return -1;
    bytesPerSecond = sampleRate * channels * ((bitsPerSample + 7) / 8);
    paused = false;
    resetPosition(0);
    return 1000;
}

void close() {
    std::lock_guard lock(clockMutex);
    bytesPerSecond = 0;
    resetPosition(0);
}

// WACUP has already sent these samples to its visualizers. Only their duration
// matters here. This output module never opens an audio device.
int write(char*, int bytes) {
    std::lock_guard lock(clockMutex);
    if (bytes < 0 || bytes > freeBufferBytes()) return 1;
    writtenBytes += bytes;
    return 0;
}

int canWrite() {
    std::lock_guard lock(clockMutex);
    return freeBufferBytes();
}

int playing() {
    std::lock_guard lock(clockMutex);
    return playedTimeMs() < writtenTimeMs();
}

int pause(int value) {
    std::lock_guard lock(clockMutex);
    bool wasPaused = paused;
    positionAtStartMs = playedTimeMs();
    paused = value != 0;
    clockStart = Clock::now();
    return wasPaused;
}

void flush(int positionMs) {
    std::lock_guard lock(clockMutex);
    resetPosition(positionMs);
}

int timePlayed() {
    std::lock_guard lock(clockMutex);
    return playedTimeMs();
}

int timeWritten() {
    std::lock_guard lock(clockMutex);
    return writtenTimeMs();
}

void empty() {}
void dialog(HWND) {}
void level(int) {}

char description[] = "Tangerine screenshot output";
Out_Module output{OUT_VER, description, 0x544e47, nullptr, nullptr, dialog, dialog, empty, empty,
                  open, close, write, canWrite, playing, pause, level, level, flush, timePlayed, timeWritten};
}

extern "C" __declspec(dllexport) Out_Module* winampGetOutModule() { return &output; }
