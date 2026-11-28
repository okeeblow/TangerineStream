#pragma once

#include <windows.h>

// The input DLL borrows the library's open profile. All buffers and the cache
// file handle are owned by the caller. No C++ objects cross the DLL boundary.
namespace tangerine {
enum class InputRequest { recognize, metadata, begin, fetch, ended };
struct PlaybackProgress {
    void* context = nullptr;
    // A null path retires this download before rename or replacement.
    void (__cdecl *update)(void*, const wchar_t*, LONGLONG, LONGLONG) = nullptr;
};
struct SourceAttempts {
    unsigned excluded = 0;
    HANDLE cancel = nullptr;
    bool temporary = false; // The input deletes this file after the decoder closes it.
    PlaybackProgress progress;
};
enum SourceStatus { unavailable, fileReady, downloadNeeded };

using InputQuery = int (__cdecl *)(InputRequest, const wchar_t*, const wchar_t*,
                                  wchar_t*, size_t, HANDLE*, SourceAttempts*);
}
