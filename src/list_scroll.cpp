#include "list_scroll.hpp"
#include "dpi.hpp"
#include "ml_sdk.hpp"
#include "tangerine_product.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <commctrl.h>
#include <memory>

namespace tangerine {
namespace {

bool scrollable(const SCROLLINFO& info) {
    return info.nPage && static_cast<long long>(info.nMax) - info.nMin + 1 > info.nPage;
}

struct ListBars {
    HWND list, player;
    HWND bars[2]{};
    HWND surfaces[2]{};
    HBITMAP skin{};
    bool pending = false;
    int wheelRemainder = 0;
    UINT sync = RegisterWindowMessageW(TS_PRODUCT_NAME_W L".SyncListScrollbars");
    UINT directWheel = RegisterWindowMessageW(L"WINAMP_WM_DIRECT_MOUSE_WHEEL");

    void destroyControls() {
        for (auto surface : surfaces) if (surface) DestroyWindow(surface);
        for (auto bar : bars) if (bar) DestroyWindow(bar);
    }

    void loadSkin() {
        if (skin) DeleteObject(skin);
        const auto bitmap = SendMessageW(player, WM_WA_IPC, 0, IPC_GET_GENSKINBITMAP);
        skin = bitmap != 1 ? reinterpret_cast<HBITMAP>(bitmap) : nullptr;
        for (int axis : {SB_HORZ, SB_VERT}) {
            HWND bar = bars[axis], surface = surfaces[axis];
            ShowWindow(surface, skin && (GetWindowLongPtrW(bar, GWL_STYLE) & WS_VISIBLE) ? SW_SHOWNA : SW_HIDE);
            InvalidateRect(skin ? surface : bar, nullptr, FALSE);
        }
    }

    void schedule() {
        if (!pending) pending = PostMessageW(list, sync, 0, 0) != FALSE;
    }

    void update() {
        RECT client{};
        GetClientRect(list, &client);
        SCROLLINFO range[2]{};
        for (int axis : {SB_HORZ, SB_VERT}) {
            range[axis].cbSize = sizeof(SCROLLINFO);
            range[axis].fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            GetScrollInfo(list, axis, &range[axis]);
        }
        const auto dpi = windowDpi(list);
        const int right = std::max(0L, client.right -
            (scrollable(range[SB_VERT]) ? systemMetricForDpi(SM_CXVSCROLL, dpi) : 0));
        // The skinned list can keep a horizontal page from before the window
        // was resized. The visible page is the space beside our vertical bar.
        auto& horizontal = range[SB_HORZ];
        horizontal.nPage = static_cast<UINT>(right);
        RECT extent{};
        const auto view = GetWindowLongPtrW(list, GWL_STYLE) & LVS_TYPEMASK;
        if (view == LVS_ICON && ListView_GetViewRect(list, &extent)) {
            horizontal.nMax = std::max(0L, extent.right + horizontal.nPos - 1);
        } else if (view == LVS_REPORT) {
            const int columns = Header_GetItemCount(ListView_GetHeader(list));
            int width = 0;
            for (int i = 0; i < columns; ++i) width += ListView_GetColumnWidth(list, i);
            horizontal.nMax = std::max(0, width - 1);
        }
        const int bottom = client.bottom - (scrollable(horizontal) ? systemMetricForDpi(SM_CYHSCROLL, dpi) : 0);
        for (int axis : {SB_HORZ, SB_VERT}) {
            const HWND bar = bars[axis];
            const RECT strip = axis == SB_VERT ? RECT{right, 0, client.right, bottom}
                                               : RECT{0, bottom, right, client.bottom};
            auto& info = range[axis];
            if (!scrollable(info) || IsRectEmpty(&strip)) {
                if (GetWindowLongPtrW(bar, GWL_STYLE) & WS_VISIBLE) ShowWindow(bar, SW_HIDE);
                if (GetWindowLongPtrW(surfaces[axis], GWL_STYLE) & WS_VISIBLE) ShowWindow(surfaces[axis], SW_HIDE);
                continue;
            }
            RECT previous{};
            GetWindowRect(bar, &previous);
            MapWindowPoints(nullptr, list, reinterpret_cast<POINT*>(&previous), 2);
            const bool moved = !(GetWindowLongPtrW(bar, GWL_STYLE) & WS_VISIBLE) || !EqualRect(&previous, &strip);
            if (moved) {
                SetWindowPos(bar, HWND_TOP, strip.left, strip.top, strip.right - strip.left, strip.bottom - strip.top,
                             SWP_NOACTIVATE | SWP_SHOWWINDOW);
                SetWindowPos(surfaces[axis], HWND_TOP, strip.left, strip.top, strip.right - strip.left, strip.bottom - strip.top,
                             SWP_NOACTIVATE | (skin ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
            }
            SCROLLINFO old{};
            old.cbSize = sizeof(old);
            old.fMask = info.fMask;
            if (moved || !GetScrollInfo(bar, SB_CTL, &old) || old.nMin != info.nMin || old.nMax != info.nMax ||
                old.nPage != info.nPage || old.nPos != info.nPos) {
                SetScrollInfo(bar, SB_CTL, &info, TRUE);
                if (skin) InvalidateRect(surfaces[axis], nullptr, FALSE);
            }
        }
    }

    void scroll(int axis, UINT code) {
        if (code == SB_THUMBTRACK || code == SB_THUMBPOSITION) {
            SCROLLINFO from{}, to{};
            from.cbSize = to.cbSize = sizeof(SCROLLINFO);
            from.fMask = SIF_POS;
            to.fMask = SIF_TRACKPOS;
            if (!GetScrollInfo(list, axis, &from) || !GetScrollInfo(bars[axis], SB_CTL, &to)) return;
            long long delta = static_cast<long long>(to.nTrackPos) - from.nPos;
            if (axis == SB_VERT && (GetWindowLongPtrW(list, GWL_STYLE) & LVS_TYPEMASK) == LVS_REPORT) {
                RECT row{};
                if (!ListView_GetItemRect(list, 0, &row, LVIR_BOUNDS)) return;
                delta *= row.bottom - row.top;
            }
            const int pixels = static_cast<int>(std::clamp<long long>(delta, INT_MIN, INT_MAX));
            ListView_Scroll(list, axis == SB_HORZ ? pixels : 0, axis == SB_VERT ? pixels : 0);
        } else {
            SendMessageW(list, axis == SB_VERT ? WM_VSCROLL : WM_HSCROLL, code, 0);
        }
        update();
    }

    void wheel(int delta) {
        // The skin hides the list's standard bars. Scroll the native list
        // directly so an inactive view can honor the system wheel setting.
        UINT lines = 3;
        SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
        if (!lines) return;

        RECT client{};
        GetClientRect(list, &client);
        RECT item{};
        if (!ListView_GetItemRect(list, 0, &item, LVIR_BOUNDS)) return;
        const bool report = (GetWindowLongPtrW(list, GWL_STYLE) & LVS_TYPEMASK) == LVS_REPORT;
        const int lineHeight = report ? std::max(1L, item.bottom - item.top)
                                      : systemMetricForDpi(SM_CYVSCROLL, windowDpi(list));
        const auto pageLines = static_cast<UINT>(std::max(1L, client.bottom / std::max(1, lineHeight)));
        const bool page = lines >= pageLines;
        const int distance = wheelRemainder + delta * static_cast<int>(page ? 1 : lines);
        const int steps = distance / WHEEL_DELTA;
        wheelRemainder = distance % WHEEL_DELTA;
        if (page) {
            for (int i = 0; i < std::abs(steps); ++i)
                SendMessageW(list, WM_VSCROLL, steps > 0 ? SB_PAGEUP : SB_PAGEDOWN, 0);
        } else if (steps) {
            ListView_Scroll(list, 0, -steps * lineHeight);
        }
    }

    // The ScrollBar control supplies the same thumb rectangle to accessibility,
    // mouse tracking and painting, including Windows' minimum thumb size.
    bool paint(HWND bar, HDC dc) const {
        SCROLLBARINFO parts{};
        parts.cbSize = sizeof(parts);
        if (!skin || !GetScrollBarInfo(bar, OBJID_CLIENT, &parts)) return false;
        RECT r{};
        GetClientRect(bar, &r);
        const bool vertical = bar == bars[SB_VERT];
        const int length = vertical ? r.bottom : r.right;
        const int arrow = parts.dxyLineButton;
        HDC atlas = CreateCompatibleDC(dc);
        if (!atlas) return false;
        auto old = SelectObject(atlas, skin);
        SetDCBrushColor(dc, GetPixel(atlas, 76, 0)); // genex scrollbar trough
        FillRect(dc, &r, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        auto draw = [&](int begin, int end, int x, int y, int w, int h) {
            if (end <= begin) return;
            StretchBlt(dc, vertical ? 0 : begin, vertical ? begin : 0,
                       vertical ? r.right : end - begin, vertical ? end - begin : r.bottom,
                       atlas, x, y, w, h, SRCCOPY);
        };
        const int y = vertical ? 31 : 45;
        draw(0, arrow, parts.rgstate[1] & STATE_SYSTEM_PRESSED ? 28 : 0, y, 14, 14);
        draw(length - arrow, length, parts.rgstate[5] & STATE_SYSTEM_PRESSED ? 42 : 14, y, 14, 14);
        const bool pressed = parts.rgstate[3] & STATE_SYSTEM_PRESSED;
        const int x = vertical ? (pressed ? 70 : 56) : 84;
        const int thumbY = vertical || !pressed ? 31 : 45;
        const int first = parts.xyThumbTop, last = std::max(first, parts.xyThumbBottom);
        // The SDK atlas contains a 14 x 28 thumb (transposed horizontally).
        // Preserve its ends and grip, stretching only the two flat spans.
        const int cap = std::min(4, (last - first) / 4);
        const int grip = std::min(10, last - first - 2 * cap);
        const int mid = first + (last - first - grip) / 2;
        auto slice = [&](int start, int end, int offset, int count) {
            draw(start, end, x + (vertical ? 0 : offset), thumbY + (vertical ? offset : 0),
                 vertical ? 14 : count, vertical ? count : 14);
        };
        slice(first, first + cap, 0, 4);
        slice(first + cap, mid, 4, 5);
        slice(mid, mid + grip, 9, 10);
        slice(mid + grip, last - cap, 19, 5);
        slice(last - cap, last, 24, 4);
        SelectObject(atlas, old);
        DeleteDC(atlas);
        return true;
    }
};

LRESULT CALLBACK surfaceProc(HWND surface, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR data) {
    auto& state = *reinterpret_cast<ListBars*>(data);
    const HWND bar = state.bars[surface == state.surfaces[SB_VERT] ? SB_VERT : SB_HORZ];
    if (msg == WM_NCHITTEST) return HTTRANSPARENT;
    if (msg == WM_ERASEBKGND) return TRUE;
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps{};
        auto dc = BeginPaint(surface, &ps);
        state.paint(bar, dc);
        EndPaint(surface, &ps);
        return 0;
    }
    if ((msg == WM_PRINTCLIENT || (msg == WM_PRINT && (l & PRF_CLIENT))) &&
        state.paint(bar, reinterpret_cast<HDC>(w))) return 0;
    return DefSubclassProc(surface, msg, w, l);
}

LRESULT CALLBACK barProc(HWND bar, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR data) {
    auto& state = *reinterpret_cast<ListBars*>(data);
    if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL || (state.directWheel && msg == state.directWheel))
        return SendMessageW(state.list, msg, w, l);
    if (state.skin && (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_CAPTURECHANGED))
        InvalidateRect(state.surfaces[bar == state.bars[SB_VERT] ? SB_VERT : SB_HORZ], nullptr, FALSE);
    return DefSubclassProc(bar, msg, w, l);
}

LRESULT CALLBACK listProc(HWND list, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data) {
    auto& state = *reinterpret_cast<ListBars*>(data);
    const bool redirected = state.directWheel && msg == state.directWheel;
    if (redirected || msg == WM_MOUSEWHEEL) {
        state.wheel(GET_WHEEL_DELTA_WPARAM(w));
        state.schedule();
        return redirected ? TRUE : 0;
    }
    if ((msg == WM_HSCROLL || msg == WM_VSCROLL) && l) {
        const int axis = msg == WM_VSCROLL ? SB_VERT : SB_HORZ;
        if (reinterpret_cast<HWND>(l) == state.bars[axis]) {
            state.scroll(axis, LOWORD(w));
            return 0;
        }
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(list, listProc, id);
        if (state.skin) DeleteObject(state.skin);
        delete &state;
        return DefSubclassProc(list, msg, w, l);
    }
    if (msg == state.sync) { state.pending = false; state.update(); return 0; }
    if (msg == WM_DISPLAYCHANGE || msg == WM_THEMECHANGED || msg == WM_SYSCOLORCHANGE ||
        (msg == WM_ML_IPC && l == ML_IPC_SKINNEDWND_SKINCHANGED)) state.loadSkin();
    switch (msg) {
    case WM_PAINT: case WM_NCPAINT: case WM_WINDOWPOSCHANGED: case WM_STYLECHANGED:
    case WM_VSCROLL: case WM_HSCROLL: case WM_MOUSEHWHEEL:
    case LVM_SCROLL: case LVM_ENSUREVISIBLE: case LVM_SETITEMCOUNT: case LVM_SETCOLUMNWIDTH:
        state.schedule();
    }
    return DefSubclassProc(list, msg, w, l);
}

} // namespace

int listViewportWidth(HWND list) {
    RECT client{}, outer{};
    GetClientRect(list, &client);
    GetWindowRect(list, &outer);
    SCROLLINFO vertical{};
    vertical.cbSize = sizeof(vertical);
    vertical.fMask = SIF_RANGE | SIF_PAGE;
    // SysListView32 calculates its page with system metrics. WACUP may
    // paint a narrower scrollbar in that same window.
    int width = outer.right - outer.left;
    if (GetScrollInfo(list, SB_VERT, &vertical) && scrollable(vertical))
        width -= systemMetricForDpi(SM_CXVSCROLL, windowDpi(list));
    return std::max(0, std::min<int>(client.right, width));
}

void attachListScrollbars(HWND list, HWND player) {
    auto state = std::make_unique<ListBars>();
    state->list = list;
    state->player = player;
    const auto id = reinterpret_cast<UINT_PTR>(list);
    const auto data = reinterpret_cast<DWORD_PTR>(state.get());
    for (int axis : {SB_HORZ, SB_VERT}) {
        auto bar = CreateWindowExW(0, L"SCROLLBAR", nullptr, WS_CHILD | WS_CLIPSIBLINGS | (axis == SB_VERT ? SBS_VERT : SBS_HORZ),
                                   0, 0, 0, 0, list, nullptr, nullptr, nullptr);
        state->bars[axis] = bar;
        // The native scrollbar draws directly to its DC while tracking. A
        // higher sibling clips those draws and passes hit tests to the control.
        auto surface = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD | WS_CLIPSIBLINGS,
                                       0, 0, 0, 0, list, nullptr, nullptr, nullptr);
        state->surfaces[axis] = surface;
        if (!surface || !SetWindowSubclass(surface, surfaceProc, id, data) ||
            !bar || !SetWindowSubclass(bar, barProc, id, data)) {
            state->destroyControls();
            return;
        }
    }
    if (SetWindowSubclass(list, listProc, id, data)) {
        // Keep the skinned list and its native scroll model. A Modern skin can
        // give its nonclient scrollbar a fixed-length bitmap thumb.
        SetWindowLongPtrW(list, GWL_STYLE, GetWindowLongPtrW(list, GWL_STYLE) | WS_CLIPCHILDREN);
        // The header can span the up arrow when its columns are wide. Clip its
        // focus repaint to the part of the header visible beside the scrollbar.
        if (HWND header = ListView_GetHeader(list))
            SetWindowLongPtrW(header, GWL_STYLE, GetWindowLongPtrW(header, GWL_STYLE) | WS_CLIPSIBLINGS);
        MLSkinnedScrollWnd_ShowHorzBar(list, FALSE);
        MLSkinnedScrollWnd_ShowVertBar(list, FALSE);
        state->loadSkin();
        state.release()->schedule();
        return;
    }
    state->destroyControls();
}

} // namespace tangerine
