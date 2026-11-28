#pragma once

#include <windows.h>

namespace tangerine {

inline UINT windowDpi(HWND window) {
    using GetDpi = UINT (WINAPI*)(HWND);
    static const auto perWindow = reinterpret_cast<GetDpi>(reinterpret_cast<void*>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow")));
    if (perWindow) {
        if (const UINT dpi = perWindow(window)) return dpi;
    }
    const HDC dc = GetDC(window);
    if (!dc) return 96;
    const int dpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(window, dc);
    return dpi > 0 ? static_cast<UINT>(dpi) : 96;
}

inline int systemMetricForDpi(int metric, UINT dpi) {
    using GetMetric = int (WINAPI*)(int, UINT);
    static const auto perDpi = reinterpret_cast<GetMetric>(reinterpret_cast<void*>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetSystemMetricsForDpi")));
    return perDpi ? perDpi(metric, dpi) : GetSystemMetrics(metric);
}

}
