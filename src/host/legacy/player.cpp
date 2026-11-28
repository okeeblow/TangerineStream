#include "host/legacy/player.hpp"
#include "host/input_extensions.hpp"
#include <tlhelp32.h>
#include <cstring>
#include <memory>

namespace tangerine {
namespace {
struct CloseSnapshot { void operator()(void* handle) const { CloseHandle(handle); } };

// Inspect only modules already loaded and initialized by the player. Loading or
// initializing another copy would bypass the host's decoder configuration.
std::vector<In_Module*> loadedInputs(HWND player) {
    std::unique_ptr<void, CloseSnapshot> snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId()));
    if (snapshot.get() == INVALID_HANDLE_VALUE) { snapshot.release(); return {}; }
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    std::vector<In_Module*> result;
    for (BOOL found = Module32FirstW(snapshot.get(), &entry); found; found = Module32NextW(snapshot.get(), &entry)) {
        if (_wcsnicmp(entry.szModule, L"in_", 3)) continue;
        auto factory = reinterpret_cast<In_Module* (__cdecl*)()>(
            reinterpret_cast<void*>(GetProcAddress(entry.hModule, "winampGetInModule2")));
        if (!factory) continue;
        auto* module = factory();
        // Before 5.1, Winamp supplies hMainWindow and the host callbacks only
        // when it starts a decoder, not when it initializes the module.
        if (module && module->hDllInstance == entry.hModule &&
            (!module->hMainWindow || module->hMainWindow == player) && module->Play)
            result.push_back(module);
    }
    return result;
}


InputExtensions moduleExtensions(const In_Module& module) {
    InputExtensions result;
    if (auto* entry = module.FileExtensions)
        while (*entry) {
            addExtensions(result, entry);
            entry += strlen(entry) + 1;
            if (!*entry) break;
            entry += strlen(entry) + 1; // description
        }
    return result;
}

std::string convertFilename(const wchar_t* filename) {
    BOOL substituted = FALSE;
    const bool utf8 = GetACP() == CP_UTF8;
    const DWORD flags = utf8 ? WC_ERR_INVALID_CHARS : WC_NO_BEST_FIT_CHARS;
    auto* usedDefault = utf8 ? nullptr : &substituted;
    const int size = WideCharToMultiByte(CP_ACP, flags, filename, -1, nullptr, 0, nullptr, usedDefault);
    if (size <= 0 || substituted) return {};
    std::string result(size, '\0');
    if (!WideCharToMultiByte(CP_ACP, flags, filename, -1, result.data(), size, nullptr, usedDefault) || substituted) return {};
    result.pop_back();
    return result;
}
}

std::wstring fromHostText(const char* text) {
    if (!text || !*text) return {};
    const int size = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
    if (size <= 0) throw Error("Could not read the player's text.");
    std::wstring result(size, L'\0');
    if (!MultiByteToWideChar(CP_ACP, 0, text, -1, result.data(), size)) throw Error("Could not read the player's text.");
    result.pop_back();
    return result;
}

std::string toHostText(const wchar_t* text) {
    if (!text || !*text) return {};
    const int size = WideCharToMultiByte(CP_ACP, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) throw Error("Could not convert text for the player.");
    std::string result(size, '\0');
    if (!WideCharToMultiByte(CP_ACP, 0, text, -1, result.data(), size, nullptr, nullptr))
        throw Error("Could not convert text for the player.");
    result.pop_back();
    return result;
}

std::wstring fromHostFilename(const char* filename) {
    auto path = fromHostText(filename);
    if (path.empty()) return path;
    const DWORD size = GetLongPathNameW(path.c_str(), nullptr, 0);
    if (!size) return path;
    std::wstring expanded(size, L'\0');
    const DWORD used = GetLongPathNameW(path.c_str(), expanded.data(), size);
    if (!used || used >= size) return path;
    expanded.resize(used);
    return expanded;
}

std::string toHostFilename(const wchar_t* filename) {
    if (!filename || !*filename) return {};
    auto result = convertFilename(filename);
    if (!result.empty()) return result;
    const auto file = std::filesystem::path(filename);
    const auto parent = file.parent_path();
    const DWORD size = GetShortPathNameW(parent.c_str(), nullptr, 0);
    std::wstring shortName(size, L'\0');
    const DWORD used = size ? GetShortPathNameW(parent.c_str(), shortName.data(), size) : 0;
    if (used && used < size) {
        shortName.resize(used);
        result = convertFilename((std::filesystem::path(shortName) / file.filename()).c_str());
    }
    if (result.empty()) throw Error("This player cannot open the cache path in its Windows character set.");
    return result;
}



InputExtensions legacyInputExtensions(HWND player) {
    InputExtensions result;
    for (auto* module : loadedInputs(player)) {
        auto extensions = moduleExtensions(*module);
        result.insert(extensions.begin(), extensions.end());
    }
    return result;
}

In_Module* selectLegacyInput(HWND player, const wchar_t* filename, const In_Module* exclude) {
    const auto path = toHostFilename(filename);
    const auto dot = path.find_last_of('.');
    InputExtensions suffixes;
    if (dot != path.npos) addExtensions(suffixes, std::string_view(path).substr(dot + 1));
    const auto modules = loadedInputs(player);
    for (auto* module : modules) {
        if (module == exclude) continue;
        if (module->IsOurFile) {
            const auto recognized = (module->version & IN_UNICODE)
                ? reinterpret_cast<int (__cdecl*)(const wchar_t*)>(module->IsOurFile)(filename)
                : reinterpret_cast<int (__cdecl*)(const char*)>(module->IsOurFile)(path.c_str());
            if (recognized) return module;
        }
    }
    for (auto* module : modules)
        if (module != exclude && !suffixes.empty() && moduleExtensions(*module).count(*suffixes.begin())) return module;
    return nullptr;
}

void copyLegacyHostCallbacks(In_Module& target, const In_Module& source) {
    target.hMainWindow = source.hMainWindow;
    target.SAVSAInit = source.SAVSAInit;
    target.SAVSADeInit = source.SAVSADeInit;
    target.SAAddPCMData = source.SAAddPCMData;
    target.SAGetMode = source.SAGetMode;
    target.SAAdd = source.SAAdd;
    target.VSAAddPCMData = source.VSAAddPCMData;
    target.VSAGetMode = source.VSAGetMode;
    target.VSAAdd = source.VSAAdd;
    target.VSASetInfo = source.VSASetInfo;
    target.dsp_isactive = source.dsp_isactive;
    target.dsp_dosamples = source.dsp_dosamples;
    target.SetInfo = source.SetInfo;
}

} // namespace tangerine
