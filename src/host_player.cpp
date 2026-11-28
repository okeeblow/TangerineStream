#include "host_player.hpp"
#include <tlhelp32.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>
#include <vector>

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

void addExtensions(InputExtensions& result, std::string_view group) {
    size_t pos = 0;
    while ((pos = group.find_first_not_of(" ;*.\t", pos)) != group.npos) {
        const auto end = group.find_first_of(" ;\t", pos);
        std::string suffix(group.substr(pos, end - pos));
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        result.insert(std::move(suffix));
        if (end == group.npos) break;
        pos = end + 1;
    }
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

bool needsAnsiInputAbi() {
    // Winamp 2 loads input plug-ins before supplying hMainWindow. Read the
    // executable's version resource in memory to choose the initial ABI.
    // Unknown hosts retain the modern ABI; no global FindWindow can select a
    // different player instance on the same desktop.
    const auto executable = GetModuleHandleW(nullptr);
    const auto resource = FindResourceW(executable, MAKEINTRESOURCEW(VS_VERSION_INFO), RT_VERSION);
    if (!resource) return false;
    const auto bytes = SizeofResource(executable, resource);
    const auto* data = static_cast<const BYTE*>(LockResource(LoadResource(executable, resource)));
    constexpr size_t fixedOffset = (3 * sizeof(WORD) + sizeof(L"VS_VERSION_INFO") + 3) & ~size_t(3);
    if (!data || bytes < fixedOffset + sizeof(VS_FIXEDFILEINFO)) return false;
    VS_FIXEDFILEINFO version{};
    std::memcpy(&version, data + fixedOffset, sizeof(version));
    if (version.dwSignature != VS_FFI_SIGNATURE) return false;
    return HIWORD(version.dwFileVersionMS) == 2;
}

void HostPlayer::attach(HWND window) {
    window_ = window;
    version_ = static_cast<unsigned>(send(IPC_GETVERSION));
    unicode_ = version_ >= 0x5030;
}

LRESULT HostPlayer::send(UINT command, WPARAM value) const {
    return SendMessageW(window_, WM_WA_IPC, value, command);
}

std::wstring HostPlayer::playlistFile(int index) const {
    if (index < 0) return {};
    const auto raw = send(unicode_ ? IPC_GETPLAYLISTFILEW : IPC_GETPLAYLISTFILE, index);
    if (static_cast<UINT_PTR>(raw) <= 1) return {};
    return unicode_ ? std::wstring(reinterpret_cast<const wchar_t*>(raw)) : fromHostFilename(reinterpret_cast<const char*>(raw));
}

void HostPlayer::enqueue(const wchar_t* filename, const wchar_t* title, int seconds) const {
    if (unicode_) {
        enqueueFileWithMetaStructW item{filename, title, seconds};
        send(IPC_ENQUEUEFILEW, reinterpret_cast<WPARAM>(&item));
    } else {
        const auto name = toHostFilename(filename), caption = toHostText(title);
        enqueueFileWithMetaStruct item{name.c_str(), caption.c_str(), seconds};
        send(IPC_ENQUEUEFILE, reinterpret_cast<WPARAM>(&item));
    }
}

void HostPlayer::metadataChanged(const wchar_t* filename) const {
    if (unicode_) send(IPC_FILE_TAG_MAY_HAVE_UPDATEDW, reinterpret_cast<WPARAM>(filename));
    else if (version_ >= 0x5000) {
        const auto name = toHostFilename(filename);
        send(IPC_FILE_TAG_MAY_HAVE_UPDATED, reinterpret_cast<WPARAM>(name.c_str()));
    }
}

InputExtensions HostPlayer::inputExtensions() const {
    const auto raw = send(unicode_ ? IPC_GET_EXTLISTW : IPC_GET_EXTLIST);
    InputExtensions result;
    if (static_cast<UINT_PTR>(raw) > 1) {
        std::unique_ptr<void, decltype(&GlobalFree)> memory(reinterpret_cast<void*>(raw), GlobalFree);
        if (unicode_) {
            for (auto* entry = static_cast<const wchar_t*>(memory.get()); *entry; entry += wcslen(entry) + 1) {
                const int size = WideCharToMultiByte(CP_UTF8, 0, entry, -1, nullptr, 0, nullptr, nullptr);
                if (size <= 1) continue;
                std::string extension(size, '\0');
                if (!WideCharToMultiByte(CP_UTF8, 0, entry, -1, extension.data(), size, nullptr, nullptr)) continue;
                extension.pop_back();
                addExtensions(result, extension);
            }
        } else {
            for (auto* entry = static_cast<const char*>(memory.get()); *entry; entry += strlen(entry) + 1)
                addExtensions(result, entry);
        }
    } else if (!unicode_) {
        for (auto* module : loadedInputs(window_)) {
            auto extensions = moduleExtensions(*module);
            result.insert(extensions.begin(), extensions.end());
        }
    }
    return result;
}

In_Module* HostPlayer::selectInput(const wchar_t* filename, const In_Module* exclude) const {
    if (version_ >= 0x5050) {
        auto* module = reinterpret_cast<In_Module*>(send(IPC_CANPLAY, reinterpret_cast<WPARAM>(filename)));
        return reinterpret_cast<UINT_PTR>(module) > 1 && module != exclude ? module : nullptr;
    }
    const auto path = toHostFilename(filename);
    const auto dot = path.find_last_of('.');
    InputExtensions suffixes;
    if (dot != path.npos) addExtensions(suffixes, std::string_view(path).substr(dot + 1));
    const auto modules = loadedInputs(window_);
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

} // namespace tangerine
