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

#include "std_wnd.h"

#include <bfc/rootwnd.h>
#include <bfc/wndevent.h>

const int WS_EX_LAYERED=0x80000;
const int LWA_ALPHA=2;
#ifndef AC_SRC_ALPHA
const int AC_SRC_ALPHA=1;
#endif
const int LWA_COLORKEY=1;
const int ULW_ALPHA=2;

static int nreal=0;

static HINSTANCE gdi32instance=NULL;
static HINSTANCE user32instance=NULL;

static void (__stdcall *setLayeredWindowAttributes)(HWND, int, int, int);
static BOOL (__stdcall *updateLayeredWindow)(
  HWND hwnd,             // handle to layered window
  HDC hdcDst,            // handle to screen DC
  POINT *pptDst,         // new screen position
  SIZE *psize,           // new size of the layered window
  HDC hdcSrc,            // handle to surface DC
  POINT *pptSrc,         // layer position
  COLORREF crKey,        // color key
  BLENDFUNCTION *pblend, // blend function
  DWORD dwFlags          // options
  );
static int (WINAPI *getRandomRgn)(HDC dc, HRGN rgn, int i)=NULL;

static int checked_for_alpha_proc;
static int grrfailed=0;

static void register_wndClass(HINSTANCE hInstance);

OSWINDOWHANDLE StdWnd::createWnd(int x, int y, int w, int h, int nochild, int acceptdrops, OSWINDOWHANDLE parent, OSMODULEHANDLE module, RootWnd *rw) {

  register_wndClass(module);

  int style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
  int exstyle=0;
  if (parent == NULL) {
    exstyle |= WS_EX_TOOLWINDOW;
    style |= WS_POPUP;
  } else
    style |= WS_CHILD;

  if (nochild) style=WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

  if (acceptdrops) exstyle |= WS_EX_ACCEPTFILES;

  HWND ret = CreateWindowEx(exstyle, BASEWNDCLASSNAME, NULL, style,
    x, y, w, h, parent, NULL, module, (LPVOID)rw);
  if (ret != INVALIDOSWINDOWHANDLE) {
    nreal++;
  }
  return ret;
}

void StdWnd::destroyWnd(OSWINDOWHANDLE wnd) {
  DestroyWindow(wnd);
  nreal--;
  if (nreal == 0) {
    if (gdi32instance) FreeLibrary(gdi32instance);
    if (user32instance) FreeLibrary(user32instance);

    gdi32instance = NULL;
    user32instance = NULL;
    getRandomRgn = NULL;
  }
}

int StdWnd::isValidWnd(OSWINDOWHANDLE wnd) {
  return IsWindow(wnd);
}

void StdWnd::setLayeredWnd(OSWINDOWHANDLE wnd, int layered) {
  if (layered) {
    // have to clear and reset, can't just set
    SetWindowLong(wnd, GWL_EXSTYLE, GetWindowLong(wnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    SetWindowLong(wnd, GWL_EXSTYLE, GetWindowLong(wnd, GWL_EXSTYLE) | WS_EX_LAYERED);
  } else {
    SetWindowLong(wnd, GWL_EXSTYLE, GetWindowLong(wnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
  }
}

int StdWnd::isLayeredWnd(OSWINDOWHANDLE wnd) {
  DWORD dwLong = GetWindowLong(wnd, GWL_EXSTYLE);
  return !!(dwLong & WS_EX_LAYERED);
}

void StdWnd::setLayeredAlpha(OSWINDOWHANDLE wnd, int amount) {
  if (!isDesktopAlphaAvailable()) return;
  if (setLayeredWindowAttributes != NULL)
   (*setLayeredWindowAttributes)(wnd, RGB(0,0,0), amount, LWA_ALPHA); 
}

void StdWnd::updateLayeredWnd(OSWINDOWHANDLE wnd, int x, int y, int w, int h, HDC surfdc, int alpha) {
  if (!isDesktopAlphaAvailable()) return;
  BLENDFUNCTION blend= {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };
  POINT sp={x,y}, pt={0,0};
  SIZE ss = { w, h };
  HDC sysdc = GetDC(NULL);
  (*updateLayeredWindow)(wnd, sysdc, &sp, &ss, surfdc, &pt, 0, &blend, ULW_ALPHA);
  ReleaseDC(NULL, sysdc);
}

void StdWnd::setWndPos(OSWINDOWHANDLE wnd, int x, int y, int w, int h,
    int nozorder, int noactive, int nocopybits, int nomove, int noresize) {
  SetWindowPos(wnd, NULL, x, y, w, h,
    SWP_DEFERERASE |		// we ignore WM_SYNCPAINT anyway
    (nozorder ? SWP_NOZORDER : 0) |
    (noactive ? SWP_NOACTIVATE : 0) |
    (nocopybits ? SWP_NOCOPYBITS : 0) |
    (nomove ? SWP_NOMOVE : 0) |
    (noresize ? SWP_NOSIZE : 0) );
}

void StdWnd::bringToFront(OSWINDOWHANDLE wnd) {
  SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0,
    SWP_NOMOVE|SWP_NOSIZE|SWP_DEFERERASE|SWP_NOOWNERZORDER);
}

void StdWnd::sendToBack(OSWINDOWHANDLE wnd) {
  SetWindowPos(wnd, HWND_BOTTOM, 0, 0, 0, 0,
    SWP_NOMOVE|SWP_NOSIZE|SWP_DEFERERASE|SWP_NOOWNERZORDER);
}

int StdWnd::isWndVisible(OSWINDOWHANDLE wnd) {
  return IsWindowVisible(wnd);
}

void StdWnd::showWnd(OSWINDOWHANDLE wnd, int noactivate) {
  ShowWindow(wnd, noactivate ? SW_SHOWNA : SW_SHOWNORMAL);
}

void StdWnd::hideWnd(OSWINDOWHANDLE wnd) {
  ShowWindow(wnd, SW_HIDE);
}

int StdWnd::isPopup(OSWINDOWHANDLE wnd) {
  return !!(GetWindowLong(wnd, GWL_STYLE) & WS_POPUP);
}

void StdWnd::setEnabled(OSWINDOWHANDLE wnd, int enabled) {
  EnableWindow(wnd, enabled);
}

void StdWnd::setFocus(OSWINDOWHANDLE wnd) {
  SetFocus(wnd);
}

OSWINDOWHANDLE StdWnd::getFocus() {
  return GetFocus();
}

void StdWnd::setTopmost(OSWINDOWHANDLE wnd, int topmost) {
  SetWindowPos(wnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER);
}

void StdWnd::invalidateRect(OSWINDOWHANDLE wnd, RECT *r) {
  InvalidateRect(wnd, r, FALSE);
}

void StdWnd::invalidateRegion(OSWINDOWHANDLE wnd, OSREGIONHANDLE region) {
  InvalidateRgn(wnd, region, FALSE);
}

void StdWnd::validateRect(OSWINDOWHANDLE wnd, RECT *r) {
  ValidateRect(wnd, r);
}

void StdWnd::validateRegion(OSWINDOWHANDLE wnd, OSREGIONHANDLE region) {
  ValidateRgn(wnd, region);
}

void StdWnd::update(OSWINDOWHANDLE wnd) {
  if (wnd != NULL) UpdateWindow(wnd);
}

int StdWnd::getUpdateRect(OSWINDOWHANDLE wnd, RECT *r) {
  return GetUpdateRect(wnd, r, FALSE);
}

void StdWnd::getUpdateRegion(OSWINDOWHANDLE wnd, OSREGIONHANDLE region) {
  GetUpdateRgn(wnd, region, FALSE);
}

int StdWnd::haveGetRandomRegion() {
  // I assume linux will just return FALSE
  if (gdi32instance == NULL && !grrfailed) {
    gdi32instance=LoadLibrary("GDI32.dll");
    if (gdi32instance != NULL) {
      getRandomRgn = (int (WINAPI *)(HDC,HRGN,int)) GetProcAddress(gdi32instance,"GetRandomRgn");
      if (getRandomRgn == NULL) {
        grrfailed=1;
        FreeLibrary(gdi32instance);
        gdi32instance=NULL;
      }
    } else {
      grrfailed=1;
    }
  }
  return (getRandomRgn != NULL);
}

void StdWnd::getRandomRegion(HDC hdc, OSREGIONHANDLE region) {
  if (!haveGetRandomRegion()) return;
  (*getRandomRgn)(hdc, region, SYSRGN);
}

void StdWnd::setWndRegion(OSWINDOWHANDLE wnd, OSREGIONHANDLE region, int redraw) {
  SetWindowRgn(wnd, region, !!redraw);
}

int StdWnd::isDesktopAlphaAvailable() {
  if (user32instance == NULL) user32instance = LoadLibrary("USER32.DLL");
  if (user32instance != NULL) {

    if (setLayeredWindowAttributes == NULL || updateLayeredWindow == NULL) {
      if (!checked_for_alpha_proc) {
        setLayeredWindowAttributes=(void (__stdcall *)(HWND,int,int,int))GetProcAddress(user32instance ,"SetLayeredWindowAttributes");
        updateLayeredWindow=(BOOL (__stdcall *)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD))GetProcAddress(user32instance ,"UpdateLayeredWindow");
        checked_for_alpha_proc = 1;
      }
    }
  }
  return (updateLayeredWindow != NULL);
}

int StdWnd::isTransparencyAvailable() {
// there is no win32 implementation that supports setLayeredWindowAttributes but not updateLayeredWindow
  return StdWnd::isDesktopAlphaAvailable();
}

void StdWnd::getClientRect(OSWINDOWHANDLE wnd, RECT *r) {
  GetClientRect(wnd, r);
}

void StdWnd::getWindowRect(OSWINDOWHANDLE wnd, RECT *r) {
  GetWindowRect(wnd, r);
}

void StdWnd::clientToScreen(OSWINDOWHANDLE wnd, int *x, int *y) {
  POINT p = { x ? *x : 0, y ? *y : 0 };
  ClientToScreen(wnd, &p);
  if (x) *x = p.x;
  if (y) *y = p.y;
}

void StdWnd::screenToClient(OSWINDOWHANDLE wnd, int *x, int *y) {
  POINT p = { x ? *x : 0, y ? *y : 0 };
  ScreenToClient(wnd, &p);
  if (x) *x = p.x;
  if (y) *y = p.y;
}

void StdWnd::setParent(OSWINDOWHANDLE child, OSWINDOWHANDLE newparent) {
  SetParent(child, newparent);
}

OSWINDOWHANDLE StdWnd::getParent(OSWINDOWHANDLE wnd) {
  return GetParent(wnd);
}

OSWINDOWHANDLE StdWnd::getTopmostChild(OSWINDOWHANDLE wnd) {
  return GetWindow(wnd, GW_CHILD);
}

void StdWnd::setCapture(OSWINDOWHANDLE wnd) {
  SetCapture(wnd);
}

void StdWnd::releaseCapture() {
  ReleaseCapture();
}

OSWINDOWHANDLE StdWnd::getCapture() {
  return GetCapture();
}

void StdWnd::revokeDragNDrop(OSWINDOWHANDLE wnd) {
  if (wnd == NULL) return;
  RevokeDragDrop(wnd);
}

void StdWnd::setWndName(OSWINDOWHANDLE wnd, const char *name) {
  SetWindowText(wnd, name);
}

void StdWnd::getWndName(OSWINDOWHANDLE wnd, char *name, int maxlen) {
  GetWindowText(wnd, name, maxlen);
}

void StdWnd::setIcon(OSWINDOWHANDLE wnd, OSICONHANDLE icon, int large) {
  SendMessage(wnd, WM_SETICON, !large ? ICON_SMALL : ICON_BIG, (int)icon);
}

OSWINDOWHANDLE StdWnd::getActiveWindow() {
  return GetActiveWindow();
}

void StdWnd::setActiveWindow(OSWINDOWHANDLE wnd) {
  SetActiveWindow(wnd);
}

//////////////////////////

static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  // fetch out the RootWnd *
  if (uMsg == WM_CREATE) {
    CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
    ASSERT(cs->lpCreateParams != NULL);
    // stash pointer to self
    SetWindowLong(hWnd, GWL_USERDATA, (LONG)cs->lpCreateParams);
  }

  // pass the messages into the BaseWnd
  RootWnd *rootwnd = (RootWnd*)GetWindowLong(hWnd, GWL_USERDATA);
  if (rootwnd == NULL) {
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  } else {
    int r = 0;
    switch (uMsg) {
      case WM_CLOSE: return 0;
      case WM_NCPAINT: return 0;
      case WM_SYNCPAINT: return 0;
//      case WM_SETFOCUS: r = rootwnd->windowEvent(WndEvent::SETFOCUS);
//      case WM_KILLFOCUS: r = rootwnd->windowEvent(WndEvent::KILLFOCUS);
      default:
        r = rootwnd->wndProc(hWnd, uMsg, wParam, lParam);
    }
    if (IsWindow(hWnd)) // wndProc may have switched skin and killed this window
      rootwnd->performBatchProcesses();
    return r;
  }
}

static void register_wndClass(HINSTANCE hInstance) {
  WNDCLASS wc;
  if (GetClassInfo(hInstance, BASEWNDCLASSNAME, &wc)) return;

  // regiszter pane class
  wc.style = 0;
  wc.lpfnWndProc = (WNDPROC)wndProc;
  wc.cbClsExtra		= 0;
  wc.cbWndExtra		= 0;
  wc.hInstance		= hInstance;
  wc.hIcon		= NULL;
  wc.hCursor		= NULL;
  wc.hbrBackground	= (HBRUSH)GetStockObject(NULL_BRUSH);
  wc.lpszMenuName	= NULL;
  wc.lpszClassName	= BASEWNDCLASSNAME;

  int r = RegisterClass(&wc);
  if (r == 0) {
    int res = GetLastError();
    if (res != ERROR_CLASS_ALREADY_EXISTS) {
      char florp[WA_MAX_PATH];
      SPRINTF(florp, "Failed to register class, err %d", res);
      ASSERTPR(0, florp);
    }
  }
}
