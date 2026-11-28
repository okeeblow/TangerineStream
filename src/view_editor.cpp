#include "view_editor.hpp"
#include "list_scroll.hpp"
#include "ml_sdk.hpp"
#include "resources.h"
#include <commctrl.h>
#include <algorithm>
#include <cwchar>

namespace tangerine {
namespace {
struct Editor {
    ViewEditorHost host;
    DLGPROC formProc;
    int formId;
    std::wstring caption;
};
INT_PTR CALLBACK frameProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_INITDIALOG) {
        auto& editor = *reinterpret_cast<Editor*>(l);
        SetWindowTextW(hwnd, editor.caption.c_str());
        HWND form = CreateDialogParamW(editor.host.instance, MAKEINTRESOURCEW(editor.formId), hwnd, editor.formProc, l);
        if (!form) { EndDialog(hwnd, -1); return TRUE; }
        SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(form));
        return TRUE;
    }
    if (msg == WM_CLOSE || (msg == WM_COMMAND && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL))) {
        auto form = reinterpret_cast<HWND>(GetWindowLongPtrW(hwnd, DWLP_USER));
        if (form) return SendMessageW(form, WM_COMMAND, msg == WM_CLOSE ? IDCANCEL : w, l);
    }
    return FALSE;
}

void skinForm(HWND hwnd, Editor& editor) {
    editor.host.skin(hwnd, SKINNEDWND_TYPE_DIALOG, 0);
    for (HWND child = GetWindow(hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        wchar_t cls[32]{};
        GetClassNameW(child, cls, 32);
        if (!lstrcmpiW(cls, L"Button")) editor.host.skin(child, SKINNEDWND_TYPE_BUTTON, 0);
        else if (!lstrcmpiW(cls, L"ComboBox")) editor.host.skin(child, SKINNEDWND_TYPE_COMBOBOX, 0);
        else if (!lstrcmpiW(cls, WC_LISTVIEWW)) {
            editor.host.skin(child, SKINNEDWND_TYPE_LISTVIEW, SWLVS_FULLROWSELECT | SWLVS_SELALWAYS | SWLVS_DOUBLEBUFFER);
            attachListScrollbars(child);
        } else editor.host.skin(child, SKINNEDWND_TYPE_STATIC, 0);
    }
}
void addChoice(HWND hwnd, int id, const wchar_t* label) {
    SendDlgItemMessageW(hwnd, id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
}
int choice(HWND hwnd, int id) { return static_cast<int>(SendDlgItemMessageW(hwnd, id, CB_GETCURSEL, 0, 0)); }
void selectChoice(HWND hwnd, int id, int index) { SendDlgItemMessageW(hwnd, id, CB_SETCURSEL, index, 0); }

struct LayoutEditor : Editor {
    ViewOptions options;
    Columns fields;
    bool supportsResults;
    int size;
    const std::vector<MusicPreset>& music;
    int firstMusic = 0;
    void enable(HWND hwnd) const {
        const bool covers = IsDlgButtonChecked(hwnd, IDC_COVERS) == BST_CHECKED;
        const bool filtered = IsDlgButtonChecked(hwnd, IDC_FILTERED) == BST_CHECKED;
        EnableWindow(GetDlgItem(hwnd, IDC_COVER_SIZE), covers || (filtered && options.resultArt));
        EnableWindow(GetDlgItem(hwnd, IDC_ALBUM_RESULTS), covers && supportsResults);
        EnableWindow(GetDlgItem(hwnd, IDC_ALBUMS_RIGHT), covers && supportsResults && IsDlgButtonChecked(hwnd, IDC_ALBUM_RESULTS));
        EnableWindow(GetDlgItem(hwnd, IDC_FIELD_FIRST), filtered);
        EnableWindow(GetDlgItem(hwnd, IDC_FIELD_SECOND), filtered);
        EnableWindow(GetDlgItem(hwnd, IDC_FIELD_THIRD), filtered && choice(hwnd, IDC_FIELD_SECOND) > 0);
        EnableWindow(GetDlgItem(hwnd, IDC_FILTERS_RIGHT), filtered);
    }
    void showFields(HWND hwnd, const std::vector<std::string>& keys) const {
        for (int i = 0; i < 3; ++i) {
            int sel = 0;
            if (static_cast<size_t>(i) < keys.size()) {
                const auto* field = findColumn(fields, keys[i]);
                if (field) sel = static_cast<int>(field - fields.data()) + (i != 0);
            }
            selectChoice(hwnd, IDC_FIELD_FIRST + i, sel);
        }
    }
};
INT_PTR CALLBACK layoutProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto* editor = reinterpret_cast<LayoutEditor*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    if (msg == WM_INITDIALOG) {
        editor = static_cast<LayoutEditor*>(reinterpret_cast<Editor*>(l));
        SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(editor));
        skinForm(hwnd, *editor);
        for (int i = 0; i < 3; ++i) {
            if (i) addChoice(hwnd, IDC_FIELD_FIRST + i, L"(none)");
            for (auto& f : editor->fields) addChoice(hwnd, IDC_FIELD_FIRST + i, f.label);
        }
        for (auto label : {L"Custom", L"Simple list", L"Album covers"}) addChoice(hwnd, IDC_PRESET, label);
        if (findColumn(editor->fields, "album") && findColumn(editor->fields, "genre")) {
            addChoice(hwnd, IDC_PRESET, L"Artist / Album");
            addChoice(hwnd, IDC_PRESET, L"Genre / Artist / Album");
        }
        editor->firstMusic = static_cast<int>(SendDlgItemMessageW(hwnd, IDC_PRESET, CB_GETCOUNT, 0, 0));
        int current = 0;
        for (size_t i = 0; i < editor->music.size(); ++i) {
            const auto& preset = editor->music[i];
            addChoice(hwnd, IDC_PRESET, preset.label);
            if (editor->options.musicPreset == preset.key) current = editor->firstMusic + static_cast<int>(i);
        }
        selectChoice(hwnd, IDC_PRESET, current);
        for (auto size : {L"Automatic", L"100", L"200", L"300", L"400", L"600"}) addChoice(hwnd, IDC_COVER_SIZE, size);
        if (editor->size) SetDlgItemInt(hwnd, IDC_COVER_SIZE, editor->size, FALSE);
        else selectChoice(hwnd, IDC_COVER_SIZE, 0);
        auto& o = editor->options;
        CheckRadioButton(hwnd, IDC_SIMPLE, IDC_FILTERED, o.mode == ViewMode::covers ? IDC_COVERS :
            o.mode == ViewMode::filtered ? IDC_FILTERED : IDC_SIMPLE);
        CheckDlgButton(hwnd, IDC_ALBUM_RESULTS, o.albumResults);
        CheckDlgButton(hwnd, IDC_ALBUMS_RIGHT, o.albumsRight);
        CheckDlgButton(hwnd, IDC_FILTERS_RIGHT, o.filtersRight);
        editor->showFields(hwnd, o.filters);
        editor->enable(hwnd);
        return TRUE;
    }
    if (!editor || msg != WM_COMMAND) return FALSE;
    const int id = LOWORD(w);
    if (id == IDCANCEL) { EndDialog(GetParent(hwnd), IDCANCEL); return TRUE; }
    if (id == IDOK) {
        auto& o = editor->options;
        o.mode = IsDlgButtonChecked(hwnd, IDC_SIMPLE) ? ViewMode::list :
            IsDlgButtonChecked(hwnd, IDC_COVERS) ? ViewMode::covers : ViewMode::filtered;
        if (IsWindowEnabled(GetDlgItem(hwnd, IDC_COVER_SIZE))) {
            wchar_t value[32]{};
            GetDlgItemTextW(hwnd, IDC_COVER_SIZE, value, 32);
            wchar_t* end = nullptr;
            long size = wcstol(value, &end, 10);
            if (lstrcmpiW(value, L"Automatic") == 0) editor->size = 0;
            else if (end != value && !*end && size >= 100 && size <= 600) editor->size = static_cast<int>(size);
            else {
                MessageBoxW(hwnd, L"Enter a cover size from 100 to 600 pixels, or choose Automatic.", L"Cover size", MB_OK);
                SetFocus(GetDlgItem(hwnd, IDC_COVER_SIZE));
                return TRUE;
            }
        }
        o.albumResults = IsDlgButtonChecked(hwnd, IDC_ALBUM_RESULTS) != 0;
        o.albumsRight = IsDlgButtonChecked(hwnd, IDC_ALBUMS_RIGHT) != 0;
        o.filtersRight = IsDlgButtonChecked(hwnd, IDC_FILTERS_RIGHT) != 0;
        std::vector<std::string> keys;
        for (int i = 0; i < 3; ++i) {
            int selected = choice(hwnd, IDC_FIELD_FIRST + i) - (i != 0);
            if (selected < 0) break;
            std::string key = editor->fields.at(selected).key;
            if (std::find(keys.begin(), keys.end(), key) != keys.end()) {
                MessageBoxW(hwnd, L"Choose a different field for each filter.", L"Filter fields", MB_OK);
                SetFocus(GetDlgItem(hwnd, IDC_FIELD_FIRST + i));
                return TRUE;
            }
            keys.push_back(std::move(key));
        }
        o.filters = std::move(keys);
        EndDialog(GetParent(hwnd), IDOK);
        return TRUE;
    }
    if (id == IDC_PRESET && HIWORD(w) == CBN_SELCHANGE) {
        int preset = choice(hwnd, id);
        bool music = preset >= editor->firstMusic && static_cast<size_t>(preset - editor->firstMusic) < editor->music.size();
        editor->options.musicPreset = music ? editor->music[preset - editor->firstMusic].key : "";
        if (preset) CheckRadioButton(hwnd, IDC_SIMPLE, IDC_FILTERED, music || preset == 1 ? IDC_SIMPLE :
                                     preset == 2 ? IDC_COVERS : IDC_FILTERED);
        if (preset == 3) editor->showFields(hwnd, {"artist","album"});
        if (preset == 4) editor->showFields(hwnd, {"genre","artist","album"});
    } else if (editor->options.musicPreset.empty() && (HIWORD(w) == CBN_SELCHANGE || HIWORD(w) == BN_CLICKED)) {
        selectChoice(hwnd, IDC_PRESET, 0);
    }
    editor->enable(hwnd);
    return FALSE;
}

struct ColumnEditor : Editor {
    const Columns& fields;
    const std::vector<std::string>& defaults;
    std::vector<std::string> enabled, ordered;
    void populate(HWND list, const std::vector<std::string>& selection) {
        ListView_DeleteAllItems(list);
        ordered = selection;
        for (auto& f : fields)
            if (std::find(ordered.begin(), ordered.end(), f.key) == ordered.end()) ordered.emplace_back(f.key);
        for (size_t i = 0; i < ordered.size(); ++i) {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.pszText = const_cast<wchar_t*>(findColumn(fields, ordered[i])->label);
            ListView_InsertItem(list, &item);
            ListView_SetCheckState(list, i, i < selection.size());
        }
        ListView_SetItemState(list, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_SetColumnWidth(list, 0, listViewportWidth(list));
    }
    void buttons(HWND hwnd, HWND list) const {
        int sel = ListView_GetNextItem(list, -1, LVNI_SELECTED), count = ListView_GetItemCount(list);
        EnableWindow(GetDlgItem(hwnd, IDC_MOVE_UP), sel > 0);
        EnableWindow(GetDlgItem(hwnd, IDC_MOVE_DOWN), sel >= 0 && sel + 1 < count);
        bool checked = false;
        for (int i = 0; !checked && i < count; ++i) checked = ListView_GetCheckState(list, i) != 0;
        EnableWindow(GetDlgItem(hwnd, IDOK), checked);
    }
};
INT_PTR CALLBACK columnsProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto* editor = reinterpret_cast<ColumnEditor*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    HWND list = GetDlgItem(hwnd, IDC_COLUMNS);
    if (msg == WM_INITDIALOG) {
        editor = static_cast<ColumnEditor*>(reinterpret_cast<Editor*>(l));
        SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(editor));
        skinForm(hwnd, *editor);
        ListView_SetExtendedListViewStyleEx(list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
        LVCOLUMNW column{};
        ListView_InsertColumn(list, 0, &column);
        editor->populate(list, editor->enabled);
        editor->buttons(hwnd, list);
        return TRUE;
    }
    if (!editor) return FALSE;
    if (msg == WM_NOTIFY && reinterpret_cast<NMHDR*>(l)->code == LVN_ITEMCHANGED) editor->buttons(hwnd, list);
    if (msg != WM_COMMAND) return FALSE;
    int id = LOWORD(w), count = ListView_GetItemCount(list);
    if (id == IDOK) {
        editor->enabled.clear();
        for (int i = 0; i < count; ++i) if (ListView_GetCheckState(list, i)) editor->enabled.push_back(editor->ordered[i]);
        if (editor->enabled.empty()) return TRUE;
    }
    if (id == IDOK || id == IDCANCEL) { EndDialog(GetParent(hwnd), id); return TRUE; }
    if (id == IDC_DEFAULTS) editor->populate(list, editor->defaults);
    if (id == IDC_ENABLE_ALL || id == IDC_DISABLE_ALL)
        for (int i = 0; i < count; ++i) ListView_SetCheckState(list, i, id == IDC_ENABLE_ALL);
    if (id == IDC_MOVE_UP || id == IDC_MOVE_DOWN) {
        int a = ListView_GetNextItem(list, -1, LVNI_SELECTED), b = a + (id == IDC_MOVE_UP ? -1 : 1);
        if (a >= 0 && b >= 0 && b < count) {
            bool checked = ListView_GetCheckState(list, a);
            ListView_SetCheckState(list, a, ListView_GetCheckState(list, b));
            ListView_SetCheckState(list, b, checked);
            std::swap(editor->ordered[a], editor->ordered[b]);
            for (auto i : {a,b}) ListView_SetItemText(list, i, 0, const_cast<wchar_t*>(findColumn(editor->fields, editor->ordered[i])->label));
            ListView_SetItemState(list, b, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(list, b, FALSE);
        }
    }
    editor->buttons(hwnd, list);
    return TRUE;
}
}

bool editLibraryView(ViewEditorHost host, HWND owner, BrowseKind kind, bool supportsResults, ViewOptions& options, int& size) {
    LayoutEditor editor{{host, layoutProc, IDD_VIEW_FORM, L"Edit View"}, options, filterFields(kind), supportsResults, size, musicPresets(kind)};
    if (DialogBoxParamW(host.instance, MAKEINTRESOURCEW(IDD_VIEW_EDITOR), owner, frameProc,
                        reinterpret_cast<LPARAM>(static_cast<Editor*>(&editor))) != IDOK) return false;
    options = std::move(editor.options);
    size = editor.size;
    return true;
}
bool editLibraryColumns(ViewEditorHost host, HWND owner, const std::wstring& caption, const Columns& fields,
                        const std::vector<std::string>& defaults, std::vector<std::string>& selected) {
    ColumnEditor editor{{host, columnsProc, IDD_COLUMNS_FORM, caption}, fields, defaults, selected, {}};
    if (DialogBoxParamW(host.instance, MAKEINTRESOURCEW(IDD_COLUMNS_EDITOR), owner, frameProc,
                        reinterpret_cast<LPARAM>(static_cast<Editor*>(&editor))) != IDOK) return false;
    selected = std::move(editor.enabled);
    return true;
}
} // namespace tangerine
