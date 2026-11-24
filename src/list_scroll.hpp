#pragma once

#include <windows.h>

namespace tangerine {

int listViewportWidth(HWND list);
void attachListScrollbars(HWND list, HWND player);

}
