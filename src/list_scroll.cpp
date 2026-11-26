#include "list_scroll.hpp"
#include "dpi.hpp"
#include "ml_sdk.hpp"

#include <algorithm>
#include <climits>
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
    HBITMAP skin{};
    bool pending = false;
    UINT sync = RegisterWindowMessageW(L"Tangerine Stream.SyncListScrollbars");

    void loadSkin() {
        if (skin) DeleteObject(skin);
        const auto bitmap = SendMessageW(player, WM_WA_IPC, 0, IPC_GET_GENSKINBITMAP);
        skin = bitmap != 1 ? reinterpret_cast<HBITMAP>(bitmap) : nullptr;
        for (auto bar : bars) InvalidateRect(bar, nullptr, FALSE);
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
        // Icon view can retain the previous header width as its horizontal
        // range after reflow. Its view rectangle contains the current extent.
        RECT extent{};
        if ((GetWindowLongPtrW(list, GWL_STYLE) & LVS_TYPEMASK) == LVS_ICON &&
            ListView_GetViewRect(list, &extent)) {
            auto& horizontal = range[SB_HORZ];
            horizontal.nMax = std::max(0L, extent.right + horizontal.nPos - 1);
        }
        const auto dpi = windowDpi(list);
        const int right = client.right - (scrollable(range[SB_VERT]) ? systemMetricForDpi(SM_CXVSCROLL, dpi) : 0);
        const int bottom = client.bottom - (scrollable(range[SB_HORZ]) ? systemMetricForDpi(SM_CYHSCROLL, dpi) : 0);
        for (int axis : {SB_HORZ, SB_VERT}) {
            const HWND bar = bars[axis];
            const RECT strip = axis == SB_VERT ? RECT{right, 0, client.right, bottom}
                                               : RECT{0, bottom, right, client.bottom};
            auto& info = range[axis];
            if (!scrollable(info) || IsRectEmpty(&strip)) {
                if (GetWindowLongPtrW(bar, GWL_STYLE) & WS_VISIBLE) ShowWindow(bar, SW_HIDE);
                continue;
            }
            RECT previous{};
            GetWindowRect(bar, &previous);
            MapWindowPoints(nullptr, list, reinterpret_cast<POINT*>(&previous), 2);
            const bool moved = !(GetWindowLongPtrW(bar, GWL_STYLE) & WS_VISIBLE) || !EqualRect(&previous, &strip);
            if (moved)
                SetWindowPos(bar, HWND_TOP, strip.left, strip.top, strip.right - strip.left, strip.bottom - strip.top,
                             SWP_NOACTIVATE | SWP_SHOWWINDOW);
            SCROLLINFO old{};
            old.cbSize = sizeof(old);
            old.fMask = info.fMask;
            if (moved || !GetScrollInfo(bar, SB_CTL, &old) || old.nMin != info.nMin || old.nMax != info.nMax ||
                old.nPage != info.nPage || old.nPos != info.nPos) {
                SetScrollInfo(bar, SB_CTL, &info, TRUE);
                InvalidateRect(bar, nullptr, FALSE);
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

    // The ScrollBar control supplies the same thumb rectangle to accessibility,
    // mouse tracking and painting, including Windows' minimum thumb size.
    bool paint(HWND bar, HDC dc) const {
        SCROLLBARINFO parts{};
        parts.cbSize = sizeof(parts);
        if (!skin || !GetScrollBarInfo(bar, OBJID_CLIENT, &parts) || parts.xyThumbBottom <= parts.xyThumbTop) return false;
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
        const int first = parts.xyThumbTop, last = parts.xyThumbBottom;
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

LRESULT CALLBACK barProc(HWND bar, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR data) {
    auto& state = *reinterpret_cast<ListBars*>(data);
    if (msg == WM_PAINT && state.skin) {
        PAINTSTRUCT ps{};
        auto dc = BeginPaint(bar, &ps);
        if (!state.paint(bar, dc)) DefSubclassProc(bar, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
        EndPaint(bar, &ps);
        return 0;
    }
    if ((msg == WM_PRINTCLIENT || (msg == WM_PRINT && (l & PRF_CLIENT))) &&
        state.paint(bar, reinterpret_cast<HDC>(w))) return 0;
    if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) return SendMessageW(state.list, msg, w, l);
    return DefSubclassProc(bar, msg, w, l);
}

LRESULT CALLBACK listProc(HWND list, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data) {
    auto& state = *reinterpret_cast<ListBars*>(data);
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
    case WM_VSCROLL: case WM_HSCROLL: case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
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
        if (!bar || !SetWindowSubclass(bar, barProc, id, data)) {
            for (auto child : state->bars) if (child) DestroyWindow(child);
            return;
        }
    }
    if (SetWindowSubclass(list, listProc, id, data)) {
        // Keep the skinned list and its native scroll model. A Modern skin can
        // give its nonclient scrollbar a fixed-length bitmap thumb.
        SetWindowLongPtrW(list, GWL_STYLE, GetWindowLongPtrW(list, GWL_STYLE) | WS_CLIPCHILDREN);
        MLSkinnedScrollWnd_ShowHorzBar(list, FALSE);
        MLSkinnedScrollWnd_ShowVertBar(list, FALSE);
        state->loadSkin();
        state.release()->schedule();
        return;
    }
    for (auto bar : state->bars) if (bar) DestroyWindow(bar);
}

} // namespace tangerine
