#include "host/legacy/input_abi.hpp"
#include "host/legacy/player.hpp"
#include "tangerine_product.h"
#include <cstring>
#include <vector>

namespace {
void (*wideFileInfo)(const wchar_t*, wchar_t*, int*) = nullptr;
int (*widePlay)(const wchar_t*) = nullptr;
int (*wideRecognize)(const wchar_t*) = nullptr;

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

void getFileInfoAnsi(const char* filename, char* title, int* length) {
    try {
        const auto file = tangerine::fromHostFilename(filename);
        wchar_t caption[GETFILEINFO_TITLE_LENGTH]{};
        wideFileInfo(file.c_str(), title ? caption : nullptr, length);
        if (title) lstrcpynA(title, tangerine::toHostText(caption).c_str(), GETFILEINFO_TITLE_LENGTH);
    } catch (...) {
        if (title) *title = 0;
        if (length) *length = -1;
    }
}

int playAnsi(const char* filename) {
    try { return widePlay(tangerine::fromHostFilename(filename).c_str()); }
    catch (...) { return 1; }
}

int recognizeAnsi(const char* filename) {
    try { return wideRecognize(tangerine::fromHostFilename(filename).c_str()); }
    catch (...) { return 0; }
}
}

namespace tangerine {
void adaptLegacyInputAbi(In_Module& input) {
    if (!needsAnsiInputAbi()) return;
    wideFileInfo = input.GetFileInfo;
    widePlay = input.Play;
    wideRecognize = input.IsOurFile;
    static char description[] = TS_PRODUCT_NAME " Input v" TS_VERSION_STRING;
    input.version = 0x100; // Original ANSI description and callbacks; void Init on the host side.
    input.description = description;
    input.GetFileInfo = reinterpret_cast<decltype(input.GetFileInfo)>(getFileInfoAnsi);
    input.InfoBox = reinterpret_cast<decltype(input.InfoBox)>(+[](const char*, HWND) { return 1; });
    input.IsOurFile = reinterpret_cast<decltype(input.IsOurFile)>(recognizeAnsi);
    input.Play = reinterpret_cast<decltype(input.Play)>(playAnsi);
}
}

extern "C" int winampGetExtendedFileInfoW(const wchar_t*, const char*, wchar_t*, int);

extern "C" __declspec(dllexport) int winampGetExtendedFileInfo(
    const char* filename, const char* field, char* output, int capacity) {
    if (!output || capacity <= 0) return 0;
    try {
        const auto file = tangerine::fromHostFilename(filename);
        std::vector<wchar_t> value(capacity);
        if (!winampGetExtendedFileInfoW(file.c_str(), field, value.data(), capacity)) return 0;
        lstrcpynA(output, tangerine::toHostText(value.data()).c_str(), capacity);
        return 1;
    } catch (...) { return 0; }
}
