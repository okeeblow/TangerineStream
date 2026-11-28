#include "host/legacy/skin.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <uxtheme.h>

namespace tangerine {
namespace {
constexpr UINT_PTR skinSubclass = 0x54534b49;
const UINT applySkin = RegisterWindowMessageW(L"Tangerine Stream.ApplyLegacySkin");

struct ControlSkin { HWND window; UINT type; LRESULT handle = 0; bool applied = false; };
struct DialogSkin {
    HWND library, player;
    int (*color)(int);
    INT_PTR (*dialogMessage)(HWND, UINT, WPARAM, LPARAM);
    HBITMAP buttons = nullptr;
    std::vector<ControlSkin> controls;

    ~DialogSkin() { if (buttons) DeleteObject(buttons); }
    void loadButtons() {
        if (buttons) DeleteObject(buttons);
        const auto raw = SendMessageW(player, WM_WA_IPC, 0, IPC_GET_GENSKINBITMAP);
        buttons = raw > 1 ? reinterpret_cast<HBITMAP>(raw) : nullptr;
    }

    LRESULT send(UINT command, WPARAM value) const {
        return SendMessageW(library, WM_ML_IPC, value, command);
    }
    void palette(HWND list) const {
        if (!color) return;
        ListView_SetBkColor(list, color(0));
        ListView_SetTextBkColor(list, color(0));
        ListView_SetTextColor(list, color(1));
    }
    void button(const DRAWITEMSTRUCT& item) const {
        const int saved = SaveDC(item.hDC);
        auto font = reinterpret_cast<HFONT>(SendMessageW(item.hwndItem, WM_GETFONT, 0, 0));
        if (font) SelectObject(item.hDC, font);
        const bool pressed = (item.itemState & ODS_SELECTED) != 0;
        const auto image = buttons;
        RECT rect = item.rcItem;
        if (image) {
            auto source = CreateCompatibleDC(item.hDC);
            auto previous = SelectObject(source, image);
            // genex.bmp's two 47 x 15 button states use four-pixel borders.
            const int x[] = {rect.left, rect.left + 4, rect.right - 4, rect.right};
            const int y[] = {rect.top, rect.top + 4, rect.bottom - 4, rect.bottom};
            const int sx[] = {0, 4, 43, 47}, sy[] = {0, 4, 11, 15};
            SetStretchBltMode(item.hDC, COLORONCOLOR);
            for (int row = 0; row < 3; ++row) for (int col = 0; col < 3; ++col)
                StretchBlt(item.hDC, x[col], y[row], x[col + 1] - x[col], y[row + 1] - y[row],
                           source, sx[col], sy[row] + (pressed ? 15 : 0), sx[col + 1] - sx[col], sy[row + 1] - sy[row], SRCCOPY);
            SelectObject(source, previous);
            DeleteDC(source);
        } else DrawFrameControl(item.hDC, &rect, DFC_BUTTON, DFCS_BUTTONPUSH | (pressed ? DFCS_PUSHED : 0));
        std::wstring label(GetWindowTextLengthW(item.hwndItem) + 1, L'\0');
        label.resize(GetWindowTextW(item.hwndItem, label.data(), static_cast<int>(label.size())));
        SetBkMode(item.hDC, TRANSPARENT);
        const COLORREF foreground = color ? color(3) : GetSysColor(COLOR_BTNTEXT);
        const COLORREF background = color ? color(2) : GetSysColor(COLOR_BTNFACE);
        auto disabled = RGB((GetRValue(foreground) + GetRValue(background)) / 2,
                            (GetGValue(foreground) + GetGValue(background)) / 2,
                            (GetBValue(foreground) + GetBValue(background)) / 2);
        SetTextColor(item.hDC, item.itemState & ODS_DISABLED ? disabled : foreground);
        InflateRect(&rect, -4, -2);
        if (pressed) OffsetRect(&rect, 1, 1);
        const UINT flags = DT_CENTER | DT_VCENTER | DT_SINGLELINE | ((item.itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0);
        DrawTextW(item.hDC, label.c_str(), static_cast<int>(label.size()), &rect, flags);
        if ((item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT)) DrawFocusRect(item.hDC, &rect);
        RestoreDC(item.hDC, saved);
    }
};

LRESULT CALLBACK checkProc(HWND window, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data) {
    auto& skin = *reinterpret_cast<DialogSkin*>(data);
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(window, checkProc, id);
        return DefSubclassProc(window, msg, w, l);
    }
    if (msg == WM_PAINT || msg == WM_PRINTCLIENT) {
        PAINTSTRUCT paint{};
        auto dc = msg == WM_PAINT ? BeginPaint(window, &paint) : reinterpret_cast<HDC>(w);
        const int saved = SaveDC(dc);
        RECT rect{}; GetClientRect(window, &rect);
        auto brush = CreateSolidBrush(skin.color(2));
        FillRect(dc, &rect, brush); DeleteObject(brush);
        const auto style = GetWindowLongPtrW(window, GWL_STYLE) & BS_TYPEMASK;
        const bool radio = style == BS_RADIOBUTTON || style == BS_AUTORADIOBUTTON;
        const bool enabled = IsWindowEnabled(window) != FALSE;
        const auto state = SendMessageW(window, BM_GETSTATE, 0, 0);
        const int size = std::min<int>(rect.bottom, MulDiv(13, GetDeviceCaps(dc, LOGPIXELSX), 96));
        RECT glyph{0, (rect.bottom - size) / 2, size, (rect.bottom + size) / 2};
        DrawFrameControl(dc, &glyph, DFC_BUTTON, (radio ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK) |
            ((state & BST_CHECKED) ? DFCS_CHECKED : 0) | ((state & BST_PUSHED) ? DFCS_PUSHED : 0) | (enabled ? 0 : DFCS_INACTIVE));
        rect.left = size + MulDiv(4, GetDeviceCaps(dc, LOGPIXELSX), 96);
        auto font = reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0));
        if (font) SelectObject(dc, font);
        std::wstring label(GetWindowTextLengthW(window) + 1, L'\0');
        label.resize(GetWindowTextW(window, label.data(), static_cast<int>(label.size())));
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, skin.color(enabled ? 4 : 5));
        const auto ui = SendMessageW(window, WM_QUERYUISTATE, 0, 0);
        DrawTextW(dc, label.c_str(), static_cast<int>(label.size()), &rect,
                  DT_SINGLELINE | DT_VCENTER | ((ui & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0));
        if ((state & BST_FOCUS) && !(ui & UISF_HIDEFOCUS)) DrawFocusRect(dc, &rect);
        RestoreDC(dc, saved);
        if (msg == WM_PAINT) EndPaint(window, &paint);
        return 0;
    }
    const auto result = DefSubclassProc(window, msg, w, l);
    switch (msg) {
    case WM_SETFOCUS: case WM_KILLFOCUS: case WM_ENABLE: case WM_SETTEXT: case BM_SETCHECK: case BM_SETSTATE: case WM_UPDATEUISTATE:
        InvalidateRect(window, nullptr, FALSE);
    }
    return result;
}

LRESULT CALLBACK comboProc(HWND window, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data) {
    auto& skin = *reinterpret_cast<DialogSkin*>(data);
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(window, comboProc, id);
        return DefSubclassProc(window, msg, w, l);
    }
    if (msg == WM_PAINT || msg == WM_PRINTCLIENT) {
        PAINTSTRUCT paint{};
        HDC dc = msg == WM_PAINT ? BeginPaint(window, &paint) : reinterpret_cast<HDC>(w);
        const int saved = SaveDC(dc);
        COMBOBOXINFO info{};
        info.cbSize = sizeof(info);
        GetComboBoxInfo(window, &info);
        RECT rect{}; GetClientRect(window, &rect);
        auto background = CreateSolidBrush(skin.color(0));
        FillRect(dc, &rect, background);
        DeleteObject(background);
        auto border = CreateSolidBrush(skin.color(5));
        FrameRect(dc, &rect, border);
        DeleteObject(border);
        DRAWITEMSTRUCT button{};
        button.hDC = dc; button.hwndItem = window; button.rcItem = info.rcButton;
        button.itemState = (SendMessageW(window, CB_GETDROPPEDSTATE, 0, 0) ? ODS_SELECTED : 0) |
                           (IsWindowEnabled(window) ? 0 : ODS_DISABLED);
        // Draw the skin's button without a label, then its dropdown arrow.
        if (skin.buttons) {
            auto source = CreateCompatibleDC(dc);
            auto previous = SelectObject(source, skin.buttons);
            StretchBlt(dc, info.rcButton.left, info.rcButton.top, info.rcButton.right - info.rcButton.left,
                       info.rcButton.bottom - info.rcButton.top, source, 0, button.itemState & ODS_SELECTED ? 15 : 0, 47, 15, SRCCOPY);
            SelectObject(source, previous); DeleteDC(source);
        } else FillRect(dc, &info.rcButton, GetSysColorBrush(COLOR_BTNFACE));
        const int x = (info.rcButton.left + info.rcButton.right) / 2, y = (info.rcButton.top + info.rcButton.bottom) / 2;
        POINT arrow[] = {{x - 3, y - 1}, {x + 3, y - 1}, {x, y + 2}};
        auto brush = CreateSolidBrush(skin.color(3));
        SelectObject(dc, brush); SelectObject(dc, GetStockObject(NULL_PEN)); Polygon(dc, arrow, 3);
        if ((GetWindowLongPtrW(window, GWL_STYLE) & 3) == CBS_DROPDOWNLIST) {
            std::wstring label(GetWindowTextLengthW(window) + 1, L'\0');
            label.resize(GetWindowTextW(window, label.data(), static_cast<int>(label.size())));
            auto font = reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0));
            if (font) SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, skin.color(IsWindowEnabled(window) ? 1 : 5));
            auto text = info.rcItem; text.left += 3;
            DrawTextW(dc, label.c_str(), static_cast<int>(label.size()), &text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
            if (GetFocus() == window && !(SendMessageW(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) DrawFocusRect(dc, &info.rcItem);
        }
        RestoreDC(dc, saved); DeleteObject(brush);
        if (msg == WM_PAINT) EndPaint(window, &paint);
        return 0;
    }
    const auto result = DefSubclassProc(window, msg, w, l);
    switch (msg) {
    case WM_SETFOCUS: case WM_KILLFOCUS: case WM_ENABLE: case WM_SETTEXT: case CB_SETCURSEL:
    case WM_KEYDOWN: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case CB_SHOWDROPDOWN:
        InvalidateRect(window, nullptr, FALSE);
    }
    return result;
}

LRESULT CALLBACK dialogProc(HWND window, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data) {
    auto& skin = *reinterpret_cast<DialogSkin*>(data);
    if (msg == applySkin) {
        // CoolSB replaces GWLP_WNDPROC. Install it after our common-controls
        // subclasses, so unskinning restores a live chain instead of leaving
        // DefSubclassProc pointing at an already freed CoolSB instance.
        for (auto& control : skin.controls) if (!control.applied && IsWindow(control.window)) {
            control.applied = true;
            const auto command = control.type == SKINNEDWND_TYPE_LISTVIEW ? ML_IPC_SKIN_LISTVIEW : ML_IPC_SKIN_COMBOBOX;
            control.handle = skin.send(command, reinterpret_cast<WPARAM>(control.window));
            if (control.handle <= 1) control.handle = 0;
            if (control.type == SKINNEDWND_TYPE_LISTVIEW) {
                ListView_SetUnicodeFormat(control.window, TRUE);
                skin.palette(control.window);
            } else if (!control.handle && skin.color) {
                SetWindowSubclass(control.window, comboProc, skinSubclass, data);
            }
        }
        return 0;
    }
    if (msg == WM_DESTROY) {
        // The 2.90 SDK releases these from the parent WM_DESTROY, while all
        // children and their original subclass chains still exist.
        for (auto& control : skin.controls) if (control.handle) {
            skin.send(control.type == SKINNEDWND_TYPE_LISTVIEW ? ML_IPC_UNSKIN_LISTVIEW : ML_IPC_UNSKIN_COMBOBOX, control.handle);
            control.handle = 0;
        }
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(window, dialogProc, id);
        delete &skin;
        return DefSubclassProc(window, msg, w, l);
    }
    if (msg == WM_DRAWITEM && l && reinterpret_cast<DRAWITEMSTRUCT*>(l)->CtlType == ODT_BUTTON) {
        skin.button(*reinterpret_cast<DRAWITEMSTRUCT*>(l));
        return TRUE;
    }
    if (msg == WM_DISPLAYCHANGE) {
        skin.loadButtons();
        for (const auto& control : skin.controls) if (control.type == SKINNEDWND_TYPE_LISTVIEW) skin.palette(control.window);
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    if (skin.dialogMessage && msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC)
        return skin.dialogMessage(window, msg == WM_CTLCOLORBTN ? WM_CTLCOLORSTATIC : msg, w, l);
    return DefSubclassProc(window, msg, w, l);
}

DialogSkin* dialogSkin(HWND library, HWND player, HWND dialog) {
    DWORD_PTR existing = 0;
    if (GetWindowSubclass(dialog, dialogProc, skinSubclass, &existing)) return reinterpret_cast<DialogSkin*>(existing);
    auto skin = std::make_unique<DialogSkin>();
    skin->library = library;
    skin->player = player;
    auto function = [&](int index) { const auto raw = skin->send(ML_IPC_SKIN_WADLG_GETFUNC, index); return raw > 1 ? raw : 0; };
    skin->color = reinterpret_cast<decltype(skin->color)>(function(1));
    skin->dialogMessage = reinterpret_cast<decltype(skin->dialogMessage)>(function(2));
    skin->loadButtons();
    if (!SetWindowSubclass(dialog, dialogProc, skinSubclass, reinterpret_cast<DWORD_PTR>(skin.get()))) return nullptr;
    return skin.release();
}
}

bool skinLegacyControl(HWND library, HWND player, HWND control, UINT type) {
    const auto owner = type == SKINNEDWND_TYPE_DIALOG ? control : GetParent(control);
    auto* skin = dialogSkin(library, player, owner);
    if (!skin) return false;
    SetWindowTheme(control, L"", L"");
    const auto font = skin->send(ML_IPC_SKIN_WADLG_GETFUNC, 66);
    if (font > 1) SendMessageW(control, WM_SETFONT, font, TRUE);
    if (type == SKINNEDWND_TYPE_LISTVIEW || type == SKINNEDWND_TYPE_COMBOBOX) {
        if (std::none_of(skin->controls.begin(), skin->controls.end(), [&](const ControlSkin& item) { return item.window == control; }))
            skin->controls.push_back({control, type});
        if (type == SKINNEDWND_TYPE_LISTVIEW) skin->palette(control);
        PostMessageW(owner, applySkin, 0, 0);
    } else if (type == SKINNEDWND_TYPE_BUTTON) {
        const auto style = GetWindowLongPtrW(control, GWL_STYLE);
        const auto buttonType = style & BS_TYPEMASK;
        if (buttonType == BS_PUSHBUTTON || buttonType == BS_DEFPUSHBUTTON)
            SetWindowLongPtrW(control, GWL_STYLE, (style & ~BS_TYPEMASK) | BS_OWNERDRAW);
        else if (skin->color && (buttonType == BS_AUTOCHECKBOX || buttonType == BS_CHECKBOX ||
                 buttonType == BS_AUTORADIOBUTTON || buttonType == BS_RADIOBUTTON))
            SetWindowSubclass(control, checkProc, skinSubclass, reinterpret_cast<DWORD_PTR>(skin));
    }
    return type != SKINNEDWND_TYPE_DIVIDER;
}
}
