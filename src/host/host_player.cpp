#include "host/host_player.hpp"
#include "host/input_extensions.hpp"
#include <cstring>
#include <memory>

namespace tangerine {
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
        result = legacyInputExtensions(window_);
    }
    return result;
}

In_Module* HostPlayer::selectInput(const wchar_t* filename, const In_Module* exclude) const {
    if (version_ >= 0x5050) {
        auto* module = reinterpret_cast<In_Module*>(send(IPC_CANPLAY, reinterpret_cast<WPARAM>(filename)));
        return reinterpret_cast<UINT_PTR>(module) > 1 && module != exclude ? module : nullptr;
    }
    return selectLegacyInput(window_, filename, exclude);
}

} // namespace tangerine
