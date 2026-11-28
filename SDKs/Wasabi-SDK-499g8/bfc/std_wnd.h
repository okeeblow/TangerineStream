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
#ifndef _STD_WIN_H
#define _STD_WIN_H

#include <bfc/std.h>

#define ROOTSTRING "RootWnd"
#define BASEWNDCLASSNAME "BaseWindow_" ROOTSTRING

#ifdef __cplusplus

class RootWnd;

namespace StdWnd {
  COMEXP OSWINDOWHANDLE createWnd(int x, int y, int w, int h, int nochild, int acceptdrops, OSWINDOWHANDLE parent, OSMODULEHANDLE module, RootWnd *rw);
  COMEXP void destroyWnd(OSWINDOWHANDLE wnd);

  COMEXP int isValidWnd(OSWINDOWHANDLE wnd);

  COMEXP void setWndPos(OSWINDOWHANDLE wnd, int x, int y, int w, int h,
    int nozorder, int noactive, int nocopybits, int nomove, int noresize);
  COMEXP void bringToFront(OSWINDOWHANDLE wnd);
  COMEXP void sendToBack(OSWINDOWHANDLE wnd);
  COMEXP int isWndVisible(OSWINDOWHANDLE wnd);
  COMEXP void showWnd(OSWINDOWHANDLE wnd, int noactivate=FALSE);
  COMEXP void hideWnd(OSWINDOWHANDLE wnd);
  COMEXP int isPopup(OSWINDOWHANDLE wnd);
  COMEXP void setEnabled(OSWINDOWHANDLE wnd, int enabled);
  COMEXP void setFocus(OSWINDOWHANDLE wnd);
  COMEXP OSWINDOWHANDLE getFocus();
  COMEXP void setTopmost(OSWINDOWHANDLE, int topmost);

  COMEXP void invalidateRect(OSWINDOWHANDLE wnd, RECT *r=NULL);
  COMEXP void invalidateRegion(OSWINDOWHANDLE wnd, OSREGIONHANDLE region);
  COMEXP void validateRect(OSWINDOWHANDLE wnd, RECT *r=NULL);
  COMEXP void validateRegion(OSWINDOWHANDLE wnd, OSREGIONHANDLE region);
  COMEXP void update(OSWINDOWHANDLE wnd);
  COMEXP int getUpdateRect(OSWINDOWHANDLE wnd, RECT *r);
  COMEXP void getUpdateRegion(OSWINDOWHANDLE wnd, OSREGIONHANDLE region);
  COMEXP int haveGetRandomRegion();
  COMEXP void getRandomRegion(HDC hdc, OSREGIONHANDLE region);	// sorry, HDC

  COMEXP void setWndRegion(OSWINDOWHANDLE wnd, OSREGIONHANDLE region, int redraw=FALSE);

  COMEXP int isDesktopAlphaAvailable();
  COMEXP int isTransparencyAvailable();
  COMEXP void setLayeredWnd(OSWINDOWHANDLE wnd, int layered);
  COMEXP int isLayeredWnd(OSWINDOWHANDLE wnd);
  COMEXP void setLayeredAlpha(OSWINDOWHANDLE wnd, int amount);
  COMEXP void updateLayeredWnd(OSWINDOWHANDLE wnd, int x, int y, int w, int h, HDC surfdc, int alpha);

  COMEXP void getClientRect(OSWINDOWHANDLE wnd, RECT *r);
  COMEXP void getWindowRect(OSWINDOWHANDLE wnd, RECT *r);
  COMEXP void clientToScreen(OSWINDOWHANDLE wnd, int *x, int *y);
  COMEXP void screenToClient(OSWINDOWHANDLE wnd, int *x, int *y);

  COMEXP void setParent(OSWINDOWHANDLE child, OSWINDOWHANDLE newparent);
  COMEXP OSWINDOWHANDLE getParent(OSWINDOWHANDLE wnd);
//  void reparent(OSWINDOWHANDLE child, OSWINDOWHANDLE newparent);
  COMEXP OSWINDOWHANDLE getTopmostChild(OSWINDOWHANDLE wnd);

  COMEXP void setCapture(OSWINDOWHANDLE wnd);
  COMEXP void releaseCapture();
  COMEXP OSWINDOWHANDLE getCapture();

  COMEXP void revokeDragNDrop(OSWINDOWHANDLE wnd);

  COMEXP void setWndName(OSWINDOWHANDLE wnd, const char *name);
  COMEXP void getWndName(OSWINDOWHANDLE wnd, char *name, int maxlen);
  COMEXP void setIcon(OSWINDOWHANDLE wnd, OSICONHANDLE icon, int large=FALSE);

  COMEXP OSWINDOWHANDLE getActiveWindow();
  COMEXP void setActiveWindow(OSWINDOWHANDLE wnd);
};

#endif
#endif
