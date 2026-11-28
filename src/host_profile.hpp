#pragma once

#include <windows.h>
#include <filesystem>
#include <iterator>
#include <string>

// The SDK checks _MSC_VER before supplying its own intptr_t.
#ifndef _MSC_VER
#define _MSC_VER 1900
#define TS_RESTORE_COMPILER
#endif
#include <Winamp/wa_ipc.h>
#ifdef TS_RESTORE_COMPILER
#undef _MSC_VER
#undef TS_RESTORE_COMPILER
#endif

namespace tangerine {

inline std::filesystem::path settingsDirectory(HWND player) {
    constexpr UINT getIniDirectoryW = 1335; // IPC_GETINIDIRECTORYW since Winamp 5.58
    auto wide = reinterpret_cast<const wchar_t*>(SendMessageW(player, WM_WA_IPC, 0, getIniDirectoryW));
    if (reinterpret_cast<UINT_PTR>(wide) > 1 && *wide) return wide;
    auto ansi = reinterpret_cast<const char*>(SendMessageW(player, WM_WA_IPC, 0, IPC_GETINIDIRECTORY));
    if (reinterpret_cast<UINT_PTR>(ansi) <= 1 || !*ansi) return {};
    int size = MultiByteToWideChar(CP_ACP, 0, ansi, -1, nullptr, 0);
    if (size <= 1) return {};
    std::wstring directory(size, L'\0');
    if (!MultiByteToWideChar(CP_ACP, 0, ansi, -1, directory.data(), size)) return {};
    directory.pop_back();
    return directory;
}

// WACUP reads the subtitle in its Preferences list from this per-profile catalog.
inline void setDefaultPluginDescription(const std::filesystem::path& profile, const wchar_t* section,
                                        const wchar_t* dll, const wchar_t* description) {
    if (profile.empty()) return;
    const auto catalog = profile / L"plugin_descriptions.ini";
    wchar_t existing[256]{};
    GetPrivateProfileStringW(section, dll, L"", existing, static_cast<DWORD>(std::size(existing)), catalog.c_str());
    if (!*existing)
        WritePrivateProfileStringW(section, dll, description, catalog.c_str());
}

} // namespace tangerine
