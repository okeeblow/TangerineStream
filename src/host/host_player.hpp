#pragma once

#include "host/legacy/player.hpp"

namespace tangerine {

class HostPlayer {
public:
    void attach(HWND window);
    bool unicode() const { return unicode_; }
    bool lateInputCallbacks() const { return version_ < 0x5010; }
    std::wstring playlistFile(int index) const;
    void enqueue(const wchar_t* filename, const wchar_t* title, int seconds) const;
    void metadataChanged(const wchar_t* filename) const;
    InputExtensions inputExtensions() const;
    In_Module* selectInput(const wchar_t* filename, const In_Module* exclude) const;

private:
    LRESULT send(UINT command, WPARAM value = 0) const;
    HWND window_ = nullptr;
    unsigned version_ = 0;
    bool unicode_ = true;
};

} // namespace tangerine
