#include "ui/list_scroll.hpp"
#include "ui/dpi.hpp"
#include "host/ml_sdk.hpp"
#include "tangerine_product.h"

#include <algorithm>
#include <climits>
#include <commctrl.h>
#include <cstdlib>
#include <memory>

namespace tangerine {
namespace {

bool scrollable(const SCROLLINFO& info) {
    return info.nPage && static_cast<long long>(info.nMax) - info.nMin + 1 > info.nPage;
}

struct ListScroll {
    HWND list;
    int wheelRemainder = 0;
    int rowRemainder = 0;
    bool pending = false;
    bool horizontalEnabled = true;
    UINT sync = RegisterWindowMessageW(TS_PRODUCT_NAME_W L".SyncListScrollbars");
    UINT directWheel = RegisterWindowMessageW(L"WINAMP_WM_DIRECT_MOUSE_WHEEL");

    void schedule() {
        if (!pending) pending = PostMessageW(list, sync, 0, 0) != FALSE;
    }

    void fitHorizontalBar() {
        const int viewport = listViewportWidth(list);
        if (!viewport) return;
        int width = 0;
        if ((GetWindowLongPtrW(list, GWL_STYLE) & LVS_TYPEMASK) == LVS_ICON) {
            RECT extent{};
            if (!ListView_GetViewRect(list, &extent)) return;
            width = extent.right + GetScrollPos(list, SB_HORZ);
        } else {
            const int columns = Header_GetItemCount(ListView_GetHeader(list));
            for (int i = 0; i < columns; ++i) width += ListView_GetColumnWidth(list, i);
        }
        const bool show = width > viewport;
        if (show != horizontalEnabled) {
            horizontalEnabled = show;
            MLSkinnedScrollWnd_ShowHorzBar(list, show);
        }
    }

    void wheel(int delta) {
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
};

LRESULT CALLBACK listProc(HWND list, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data) {
    auto& state = *reinterpret_cast<ListScroll*>(data);
    const bool redirected = state.directWheel && msg == state.directWheel;
    if (redirected || msg == WM_MOUSEWHEEL) {
        state.wheel(GET_WHEEL_DELTA_WPARAM(w));
        return redirected ? TRUE : 0;
    }
    if (msg == LVM_SCROLL && l && (GetWindowLongPtrW(list, GWL_STYLE) & LVS_TYPEMASK) == LVS_REPORT) {
        // A precision touchpad posts pixel deltas; a report list moves in whole rows.
        RECT row{};
        if (ListView_GetItemRect(list, 0, &row, LVIR_BOUNDS) && row.bottom > row.top) {
            const int height = row.bottom - row.top;
            const long long pixels = static_cast<long long>(l) + state.rowRemainder;
            state.rowRemainder = static_cast<int>(pixels % height);
            l = static_cast<LPARAM>(std::clamp<long long>(pixels - state.rowRemainder, INT_MIN, INT_MAX));
            if (!w && !l) return TRUE;
        }
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(list, listProc, id);
        delete &state;
        return DefSubclassProc(list, msg, w, l);
    }
    if (msg == state.sync) {
        state.pending = false;
        state.fitHorizontalBar();
        return 0;
    }
    switch (msg) {
    case LVM_SETVIEW: state.rowRemainder = 0; [[fallthrough]];
    case WM_PAINT: case WM_SIZE: case WM_STYLECHANGED: case LVM_SETITEMCOUNT: case LVM_SETCOLUMNWIDTH:
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

void attachListScrollbars(HWND list) {
    auto state = std::make_unique<ListScroll>();
    state->list = list;
    if (SetWindowSubclass(list, listProc, reinterpret_cast<UINT_PTR>(list), reinterpret_cast<DWORD_PTR>(state.get()))) {
        // Windows uses the list's native vertical scrollbar to route touchpad pans.
        MLSkinnedScrollWnd_ShowVertBar(list, TRUE);
        state.release()->schedule();
    }
}

} // namespace tangerine
