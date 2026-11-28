#pragma once
#include "library_view.hpp"
#include <windows.h>

namespace tangerine {
struct ViewEditorHost {
    HINSTANCE instance;
    bool (*skin)(HWND, UINT, UINT);
};
bool editLibraryView(ViewEditorHost host, HWND owner, BrowseKind kind, bool supportsResults,
                     ViewOptions& options, int& coverSize);
bool editLibraryColumns(ViewEditorHost host, HWND owner, const std::wstring& caption,
                        const Columns& fields, const std::vector<std::string>& defaults,
                        std::vector<std::string>& selected);
}
