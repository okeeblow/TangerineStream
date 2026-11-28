#pragma once

#include <windows.h>

namespace tangerine {

bool listCanScroll(HWND list, int bar);
int listViewportWidth(HWND list);
void attachListScrollbars(HWND list);

}
