/*

  Nullsoft WASABI Source File License

  Copyright 1999-2003 Nullsoft, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

*/
#include "precomp.h"
#include "foreignwnd.h"

PtrListQuickSorted<ForeignWndProc, ForeignWndProcComparator> ForeignWnd::foreignwndprocs;

LRESULT CALLBACK foreignWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  ForeignWndProc *wp = ForeignWnd::foreignwndprocs.findItem((const char *)hwnd);
  if (wp) {
    int r = wp->wnd->wndProc(hwnd, uMsg, wParam, lParam);
    if (r) return r;
    return CallWindowProc(wp->oldWindowProc, hwnd, uMsg, wParam, lParam);
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);  
}

ForeignWnd::ForeignWnd(OSWINDOWHANDLE wndhandle, OSMODULEHANDLE module, int subclass) {
  thishwnd = wndhandle;
  setOSModuleHandle(module);

  #ifdef EXPERIMENTAL_INDEPENDENT_AOT
  DebugString("ForeignWnd::ForeignWnd(OSWINDOWHANDLE wndhandle): There might be problems with GWL_USERDATA assumptions when EXPERIMENTAL_INDEPENDENT_AOT is on, lone should audit\n");
  #endif

  // access protected basewnd member
  hwnd = wndhandle;

  setForeignWnd(1);

  if (subclass) {
    oldWindowProc = (WNDPROC) GetWindowLong(wndhandle, GWL_WNDPROC);
    wndprocentry = new ForeignWndProc;
    wndprocentry->wnd = this;
    wndprocentry->hwnd = thishwnd;
    wndprocentry->oldWindowProc = oldWindowProc;
    foreignwndprocs.addItem(wndprocentry);
    SetWindowLong(thishwnd, GWL_WNDPROC, (LONG)foreignWindowProc);
  } else {
    oldWindowProc = NULL;
    wndprocentry = NULL;
  }
}

ForeignWnd::~ForeignWnd() {
  if (wndprocentry && oldWindowProc) {
    foreignwndprocs.removeItem(wndprocentry);
    delete wndprocentry;
    wndprocentry = NULL;
    SetWindowLong(thishwnd, GWL_WNDPROC, (LONG)oldWindowProc);
    oldWindowProc = NULL;
  }
}

