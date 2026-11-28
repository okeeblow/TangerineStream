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
#ifndef __FOREIGNWND_H
#define __FOREIGNWND_H

#include "../basewnd.h"

class ForeignWnd;

class ForeignWndProc {
public:
  ForeignWnd *wnd;
  HWND hwnd;
  WNDPROC oldWindowProc;
};

class COMEXP ForeignWndProcComparator {
public:
  // comparator for sorting
  static int compareItem(ForeignWndProc *p1, ForeignWndProc* p2) {
    return CMP3(p1->hwnd, p2->hwnd);
  }
  // comparator for searching
  static int compareAttrib(const char *attrib, ForeignWndProc *item) {
    return CMP3((HWND)attrib, item->hwnd);
  }
};

class ForeignWnd : public BaseWnd {
public:
// takes over an existing OSWINDOWHANDLE and wraps a BaseWnd around it
// but does not changes the windowproc, nor does it inserts the wnd 
// into the system list. It does not either (under windows anyway) 
// sets the userdata windowlong to the object pointer.
// if subclass is true, we subclas the windowproc and process events
// as if the window was a real rootwnd

  ForeignWnd(OSWINDOWHANDLE w, OSMODULEHANDLE m, int subclass=0); 

  virtual ~ForeignWnd();

  static PtrListQuickSorted<ForeignWndProc, ForeignWndProcComparator> foreignwndprocs;

private:
  WNDPROC oldWindowProc;
  HWND thishwnd;
  ForeignWndProc *wndprocentry;
};

#endif
