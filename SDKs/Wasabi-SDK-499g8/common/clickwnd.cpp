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
#include "../bfc/api/api_wnd.h"

#include "clickwnd.h"
#include "../bfc/notifmsg.h"
#include "../common/guiobjwnd.h"

#include "../studio/api.h"

ClickWnd::ClickWnd() {
  handleRight = TRUE;
  button = -1;
  mousedown = 0;
  mcaptured = 0;
  hilite = 0;
  down = 0;
  areacheck = 0;
}

ClickWnd::~ClickWnd() {
  BaseWnd::hintDestroying(); // so basewnd doesn't call onCancelCapture
  if (getCapture()) endCapture();
}

void ClickWnd::setHandleRightClick(int tf) {
  handleRight=tf;
}

int ClickWnd::getHandleRightClick() {
  return handleRight;
}

int ClickWnd::onLeftButtonDown(int x, int y) {
  notifyParent(ChildNotify::CLICKWND_LEFTDOWN, x, y);
  CLICKWND_PARENT::onLeftButtonDown(x, y);
  abortTip();
  RootWnd *dp = getDesktopParent();
  if (dp != NULL) {
    if (dp->wantActivation()) {
      SetActiveWindow(getRootParent()->gethWnd()); 
      SetFocus(getRootParent()->gethWnd());
    } else {
      HWND w = dp->gethWnd();
      HWND owner = GetWindow(w, GW_OWNER);
      if (owner != NULL) {
        SetActiveWindow(owner);
        SetFocus(owner);
      }
    }
  } else {
    SetActiveWindow(getRootParent()->gethWnd()); 
  }
  if (ptInRegion(x, y))
    return onButtonDown(WM_LBUTTONDOWN, x, y);
  else
    return 1;
}

int ClickWnd::onRightButtonDown(int x, int y) {
  notifyParent(ChildNotify::CLICKWND_RIGHTDOWN, x, y);
  CLICKWND_PARENT::onRightButtonDown(x, y);
  abortTip();
  if (!handleRight) return 1;
  if (ptInRegion(x, y))
    return onButtonDown(WM_RBUTTONDOWN, x, y);
  else
    return 1;
}

int ClickWnd::onLeftButtonUp(int x, int y) {
  notifyParent(ChildNotify::CLICKWND_LEFTUP, x, y);
  CLICKWND_PARENT::onLeftButtonUp(x, y);
//jf
//  if (ptInRegion())
    return onButtonUp(WM_LBUTTONUP, x, y);
//  else
//    return 1;
}

int ClickWnd::onRightButtonUp(int x, int y) {
  notifyParent(ChildNotify::CLICKWND_RIGHTUP, x, y);
  CLICKWND_PARENT::onRightButtonUp(x, y);
  //jf
  //if (ptInRegion()) 
  if (!handleRight) {
    onRightPush(x, y);
    return 1;
  }
    return onButtonUp(WM_RBUTTONUP, x, y);
//  else
//    return 1;
}

int ClickWnd::onMouseMove(int x, int y) {
  POINT pos, rpos={x,y};
  int mouseover;

  CLICKWND_PARENT::onMouseMove(x, y);

  pos=rpos;
  clientToScreen(&pos);

  mouseover = (WASABI_API_WND->rootWndFromPoint(&pos) == static_cast<RootWnd *>(this) && ptInRegion(x, y));
  if (!mouseover && (!mousedown || !Std::keyDown(button?MK_RBUTTON:MK_LBUTTON))) {
    if (mcaptured || getCapture()) {
      _onLeaveArea();
      endCapture();
      mcaptured = 0;
      hilite = 0;
      down = 0;
      mousedown = 0;
      invalidate();
    }
    return 1;
  }

  if (!getCapture() && mouseover) {	// capture to see when leave
    _enterCapture();
  }

  int lastdown = down;
  int lasthilite = hilite;
  hilite = mouseover;
#ifdef WASABI_COMPILE_WNDMGR
  int m = getGuiObject() ? getGuiObject()->guiobject_getMover() : 0;
#else
  int m = 0;
#endif
  if (!m) {
    down = userDown() || (mouseover && mousedown);
  } else 
    down = userDown() || mousedown;
  
  // FG> note to self now that i finally fixed this... :
  // there is a potential bottleneck here, if for some reason this test is always true when moving the windows around like crazy.
  if (down != lastdown || (hilite != lasthilite && !m)) {
    invalidate();
  }

  return 1;
}

void ClickWnd::_enterCapture() {
  if (!mcaptured) _onEnterArea();
  if (!GetCapture()) beginCapture();     
  mcaptured = 1;
}

int ClickWnd::onButtonDown(int which, int x, int y) {
  
  if (!wantClicks()) return 1;

  if (!getCapture()) {
    _enterCapture();
  }
  mousedown = 1;
  down = 1;
  button = -1;
  if (which == WM_LBUTTONDOWN) button = 0;
  else if (which == WM_RBUTTONDOWN) button = 1;
  invalidate();

  return 1;
}

int ClickWnd::onButtonUp(int which, int x, int y) {

    // make sure same button
  if (button == 0 && which == WM_RBUTTONUP) return 1;
  if (button == 1 && which == WM_LBUTTONUP) return 1;

  if (!down) {
    if (mcaptured) _onLeaveArea();
    endCapture();
    mcaptured = 0;
    hilite = 0;
    mousedown = 0;
    return 1;
  }

    POINT pos={x,y};
    clientToScreen(&pos);

    int mouseover = (WASABI_API_WND->rootWndFromPoint(&pos) == (RootWnd *)this && ptInRegion(x, y));
    if (!mouseover && mcaptured) {
      _onLeaveArea();
      endCapture();
      mcaptured = 0;
      hilite = 0;
    }

    // it was down, process the event
    int a = down;
    down = 0;
    mousedown = 0;
    invalidate();

    if (a) {
      if (button == 0) onLeftPush(x, y);
      else if (button == 1) onRightPush(x, y);
    } else {		// was off button
      hilite = 0;
    }

    return 1;
}

void ClickWnd::onSetVisible(int show) {
  CLICKWND_PARENT::onSetVisible(show);
  if (!show && getCapture()) {
    mcaptured=0;
    hilite = 0;
    down = 0;
    mousedown = 0;
    endCapture();
    _onLeaveArea();
  }
}

void ClickWnd::_onEnterArea() {
  if (areacheck == 0) {
    onEnterArea();
    areacheck++;
  } else 
    DebugString("onEnterArea check failed\n");
}

void ClickWnd::_onLeaveArea() {
  if (areacheck == 1) {
    onLeaveArea();
    areacheck--;
  } else 
    DebugString("onLeaveArea check failed\n");
}

void ClickWnd::onEnterArea() {
}

void ClickWnd::onLeaveArea() {
}

void ClickWnd::onCancelCapture() {
  CLICKWND_PARENT::onCancelCapture();
  if (areacheck) _onLeaveArea();
  mcaptured=0;
  hilite = 0;
  down = 0;
  mousedown = 0;
}

