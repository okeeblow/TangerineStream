#pragma once

#include <windows.h>

// The input DLL borrows the library's open profile. All buffers and the cache
// file handle are owned by the caller. No C++ objects cross the DLL boundary.
namespace tangerine {
enum class InputRequest { recognize, metadata, source, ended };
using InputQuery = int (__cdecl *)(InputRequest, const wchar_t*, const wchar_t*,
                                  wchar_t*, size_t, HANDLE*);
}
