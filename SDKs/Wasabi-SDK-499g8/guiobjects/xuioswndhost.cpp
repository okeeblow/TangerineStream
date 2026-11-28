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
#include "xuioswndhost.h"
#include "../bfc/notifmsg.h"
#include "../bfc/paramparser.h"
#include "../bfc/region.h"
#include "../studio/layout.h"

// -----------------------------------------------------------------------
char OSWndHostXuiObjectStr[] = "OSWndHost"; // This is the xml tag
char OSWndHostXuiSvcName[] = "OSWndHost XuiObject Service"; 

// -----------------------------------------------------------------------
XuiOSWndHost::XuiOSWndHost() {
  setVirtual(0); // we are a real window, with an hwnd and shit
  hasregionrect = 0;
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "hwnd", XUIOSWNDHOST_SETHWND, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "offsets", XUIOSWNDHOST_SETOFFSETS, XUI_ATTRIBUTE_IMPLIED);

  ScriptObject *o = getGuiObject()->guiobject_getScriptObject();
  o->vcpu_setInterface(osWndHostGuid, static_cast<OSWndHost *>(this));

}

// -----------------------------------------------------------------------
XuiOSWndHost::~XuiOSWndHost() {
}

// -----------------------------------------------------------------------
int XuiOSWndHost::onPaint(Canvas *c) {
  XUIOSWNDHOST_PARENT::onPaint(c); 
  RECT r;
  getClientRect(&r);
  c->fillRect(&r, 0);
  return 1;
}

// -----------------------------------------------------------------------
int XuiOSWndHost::onInit() {
  XUIOSWNDHOST_PARENT::onInit();
  if (wnd != NULL) {
    onBeforeReparent(1);
    oldparent = GetParent(wnd);
    oldpopup = (GetWindowLong(wnd, GWL_STYLE) & WS_POPUP);
    oldchild = (GetWindowLong(wnd, GWL_STYLE) & WS_CHILD);
    GetWindowRect(wnd, &oldrect);
    SetWindowLong(wnd, GWL_STYLE, GetWindowLong(wnd, GWL_STYLE) | WS_CHILD);
    SetParent(wnd, gethWnd());
    onAfterReparent(1);
    dropVirtualCanvas(); // we don't need a canvas anymore, save the memory!
  }
  return 1;
}

// -----------------------------------------------------------------------
int XuiOSWndHost::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return XUIOSWNDHOST_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case  XUIOSWNDHOST_SETHWND: {
      HWND wnd = (HWND)ATOI(value);
      if (IsWindow(wnd))
        oswndhost_host(wnd);
      break;
    }
    case  XUIOSWNDHOST_SETOFFSETS: {
      ParamParser pp(value, ",");
      RECT r={0,0,0,0};
      if (pp.getNumItems() > 0)
        r.left = ATOI(pp.enumItem(0));
      if (pp.getNumItems() > 1)
        r.top = ATOI(pp.enumItem(1));
      if (pp.getNumItems() > 2)
        r.right = ATOI(pp.enumItem(2));
      if (pp.getNumItems() > 3)
        r.bottom = ATOI(pp.enumItem(3));
      oswndhost_setRegionOffsets(&r);
      break;
    }
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void XuiOSWndHost::oswndhost_setRegionOffsets(RECT *r) {
  if (r == NULL) {
    hasregionrect = 0;
    if (wnd != NULL && isPostOnInit()) {
      SetWindowRgn(wnd, NULL, 0);
      onResize();
    }
    return;
  }
  regionrect = *r;
  hasregionrect = 1;
  if (isPostOnInit())
    onResize();
}

// -----------------------------------------------------------------------
int XuiOSWndHost::onResize() {
  if (!XUIOSWNDHOST_PARENT::onResize()) return 0;

  if (wnd != NULL) {
    RECT r;
    getClientRect(&r);

    if (renderRatioActive()) {
      float ra = getRenderRatio();
      multRatio(&r);
    }

    if (hasregionrect) {
      r.left -= regionrect.left;
      r.top -= regionrect.top;
      r.right += regionrect.right;
      r.bottom += regionrect.bottom;
    }

    SetWindowPos(wnd, NULL, r.left, r.top, r.right-r.left, r.bottom-r.top, SWP_NOZORDER|SWP_NOACTIVATE);

    if (hasregionrect) {
      RECT cr={0,0,r.right-r.left,r.bottom-r.top};
      RECT wndr={cr.left+regionrect.left, cr.top+regionrect.top, cr.right-regionrect.right, cr.bottom-regionrect.bottom};
      
      RegionI reg(&wndr);
      SetWindowRgn(wnd, reg.makeWindowRegion(), 0);
    }
  }
  return 1;
}

// -----------------------------------------------------------------------
void XuiOSWndHost::oswndhost_host(HWND oswnd) {
  ASSERT(IsWindow(oswnd));
  if (isPostOnInit()) {
    onBeforeReparent(1);
    oldparent = GetParent(oswnd);
  
    // remember if WS_POPUP or WS_CHILD was set so we can reset it when we unhost
    oldpopup = (GetWindowLong(oswnd, GWL_STYLE) & WS_POPUP);
    oldchild = (GetWindowLong(oswnd, GWL_STYLE) & WS_CHILD);

    SetWindowLong(oswnd, GWL_STYLE, GetWindowLong(wnd, GWL_STYLE) | WS_CHILD);
    GetWindowRect(oswnd, &oldrect);
    SetParent(oswnd, gethWnd());
    onAfterReparent(1);
    dropVirtualCanvas(); // we don't need a canvas anymore, save the memory!
  }
  wnd = oswnd;
  if (isPostOnInit()) 
    onResize();
}

// -----------------------------------------------------------------------
void XuiOSWndHost::oswndhost_unhost() {
  if (wnd == NULL) return;
  onBeforeReparent(0);
  
  // set back the old flags
  if (oldpopup) SetWindowLong(wnd, GWL_STYLE, GetWindowLong(wnd, GWL_STYLE) | WS_POPUP);
  else SetWindowLong(wnd, GWL_STYLE, GetWindowLong(wnd, GWL_STYLE) & ~WS_POPUP);

  if (oldchild) SetWindowLong(wnd, GWL_STYLE, GetWindowLong(wnd, GWL_STYLE) | WS_CHILD);
  else SetWindowLong(wnd, GWL_STYLE, GetWindowLong(wnd, GWL_STYLE) & ~WS_CHILD);
  
  int config_aot = ((GetWindowLong(WASABI_API_WND->main_getRootWnd()->gethWnd(), GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);
  SetWindowPos(wnd, config_aot ? HWND_TOPMOST : HWND_NOTOPMOST, oldrect.left, oldrect.top, oldrect.right-oldrect.left, oldrect.bottom-oldrect.top, SWP_NOZORDER|SWP_NOACTIVATE);

  SetParent(wnd, oldparent);
  if (hasregionrect) SetWindowRgn(wnd, NULL, 0);

  onAfterReparent(0);

  hasregionrect = 0;
  wnd = NULL;
}

int XuiOSWndHost::onUserMessage(int msg, int w, int l, int *r) {
  switch (msg) {
    case OSWNDHOST_REQUEST_IDEAL_SIZE:
      postDeferredCallback(DCB_OSWNDHOST_REQUEST_IDEAL_SIZE, (int)(new DCBIdealSize(w, l)));
      *r = 1;
      return 1;
  }
  return XUIOSWNDHOST_PARENT::onUserMessage(msg, w, l, r); // do default handling
}

int XuiOSWndHost::onDeferredCallback(int p1, int p2) {
  switch (p1) {
    case DCB_OSWNDHOST_REQUEST_IDEAL_SIZE:
      DCBIdealSize *ideal = (DCBIdealSize *)p2;
      if (ideal == NULL) break;

      RootWnd *p=getDesktopParent(); // Gets the group/layout at the base of the wnd tree, the desktop hwnd 
      if (p == NULL) {
        delete ideal;
        break;
      }
      Layout *l = static_cast<Layout *>(p->getInterface(layoutGuid));
      if (l) 
        l->beginResize();
      RECT r,r2;
      getClientRect(&r); // the size of this wnd
      if (renderRatioActive()) multRatio(&r);

      p->getWindowRect(&r2); // the size of the desktop parent
      
      // take borders into account
      int neww = ideal->m_idealwidth+((r2.right-r2.left)-(r.right-r.left)); 
      int newh = ideal->m_idealheight+((r2.bottom-r2.top)-(r.bottom-r.top));
      
      // take layout min/max values into account
      int min_w = p->getPreferences(MINIMUM_W);
      int min_h = p->getPreferences(MINIMUM_H);
      int max_w = p->getPreferences(MAXIMUM_W);
      int max_h = p->getPreferences(MAXIMUM_H);
      if (min_w != AUTOWH) neww = MAX(min_w, neww);
      if (min_h != AUTOWH) newh = MAX(min_h, newh);
      if (max_w != AUTOWH) neww = MIN(max_w, neww);
      if (max_h != AUTOWH) newh = MIN(max_h, newh);

      RECT res={r2.left,r2.top,neww,newh};
      p->resize(r2.left, r2.top, neww, newh); 
      if (l) 
        l->endResize();
      delete ideal;
      return 1;
  }
  return XUIOSWNDHOST_PARENT::onDeferredCallback(p1, p2);
}

void XuiOSWndHost::onBeforeReparent(int i) {
}

void XuiOSWndHost::onAfterReparent(int i) {
}

