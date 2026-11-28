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
#ifndef __OSWndHost_H
#define __OSWndHost_H

#include "../common/guiobjwnd.h"
#include "../bfc/autobitmap.h"
#include "../bfc/wnds/oswndhost.h"

#define  XUIOSWNDHOST_PARENT GuiObjectWnd

#define DCB_OSWNDHOST_REQUEST_IDEAL_SIZE 2048
#define OSWNDHOST_REQUEST_IDEAL_SIZE WM_USER + DCB_OSWNDHOST_REQUEST_IDEAL_SIZE

class DCBIdealSize {
  public:
  DCBIdealSize(int idealwidth, int idealheight) : m_idealwidth(idealwidth), m_idealheight(idealheight) {}
  int m_idealwidth;
  int m_idealheight;
};

// -----------------------------------------------------------------------
class XuiOSWndHost : public  XUIOSWNDHOST_PARENT, public OSWndHostI {
  
  public:
                             
    XuiOSWndHost();
    virtual ~XuiOSWndHost();
    virtual int onPaint(Canvas *c);
    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual int onResize();
    virtual int onInit();
    void setHWND(const char *hwnd);
    HWND getHWND() { return wnd; }
    void setHWND(HWND hwnd) { wnd = hwnd; }
    virtual void oswndhost_host(HWND oswnd);
    virtual void oswndhost_unhost();
    virtual void oswndhost_setRegionOffsets(RECT *r);

    virtual int onUserMessage(int msg, int w, int l, int *r);
    virtual int onDeferredCallback(int p1, int p2);

    virtual void onBeforeReparent(int host);
    virtual void onAfterReparent(int host);
    virtual int handleRatio() { return 0; }
    virtual int handleDesktopAlpha() { return 0; }

  private:

    enum {
       XUIOSWNDHOST_SETHWND        = 0,
       XUIOSWNDHOST_SETOFFSETS     = 1,
    };

    int myxuihandle;
    RECT regionrect;
    int hasregionrect;
    HWND wnd;
    HWND oldparent;
    int oldpopup;
    int oldchild;
    RECT oldrect;
};


// -----------------------------------------------------------------------

extern char OSWndHostXuiObjectStr[];
extern char OSWndHostXuiSvcName[];
class OSWndHostXuiSvc : public XuiObjectSvc<XuiOSWndHost, OSWndHostXuiObjectStr, OSWndHostXuiSvcName> {};

#endif
