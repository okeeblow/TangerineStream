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
#include "xuimenu.h"
#include "studio/services/svc_action.h"

#define MENU_TIMER_CHECK 0x100
#define DC_MENU_CHAIN    0x101
#define DC_MENU_REENABLE 0x102
#define DC_MENU_INIT     0x103
#define DC_MENU_NEXT     0x104
#define DC_MENU_PREV     0x105
#define DC_MENU_OPENACTION 0x106
#define MENU_TIMER_POPKBDLOCK 0x107
#define MENU_TIMER_DELAY 5

char MenuXuiObjectStr[] = "Menu"; // This is the xml tag
char MenuXuiSvcName[] = "Menu XuiObject Service";

static XuiMenu *xuimenu_hookingMenu = NULL;
static HHOOK hhook=NULL;
static HHOOK hhook_menuselect=NULL;
static int hookusercount = 0;

XuiMenu::XuiMenu() {
  timerset = 0;
  disablespawn = 0;
  nextinchain = NULL;
  normal = NULL;
  down = NULL;
  isspawned = 0;
  inarea = 0;
  kbdhook = 0;
  kbdlocktimer = 0;
  submenu_isselected = 0;
  submenu_selectedbymouse = 0;
  submenu_selected = NULL;
  first_hmenu = NULL;
  cur_hmenu = NULL;
  getScriptObject()->vcpu_setInterface(xuiMenuGuid, (void *)static_cast<XuiMenu *>(this));
  xuihandle = newXuiHandle();
	addParam(xuihandle, "menugroup", MENU_MENUGROUP, XUI_ATTRIBUTE_IMPLIED);
	addParam(xuihandle, "menu", MENU_MENU, XUI_ATTRIBUTE_IMPLIED);
	addParam(xuihandle, "normal", MENU_NORMALID, XUI_ATTRIBUTE_IMPLIED);
	addParam(xuihandle, "down", MENU_DOWNID, XUI_ATTRIBUTE_IMPLIED);
	addParam(xuihandle, "hover", MENU_HOVERID, XUI_ATTRIBUTE_IMPLIED);
	addParam(xuihandle, "next", MENU_NEXT, XUI_ATTRIBUTE_IMPLIED);
	addParam(xuihandle, "prev", MENU_PREV, XUI_ATTRIBUTE_IMPLIED);
}

XuiMenu::~XuiMenu() {
  if (timerset) killTimer(MENU_TIMER_CHECK);
  if (kbdlocktimer) { killTimer(MENU_TIMER_POPKBDLOCK); WASABI_API_WND->popKeyboardLock(); } // do not set kbdlocktimer to 0, or stopKbdHook will start a timer
  stopKbdHook();
}

int XuiMenu::onInit() {
  XUIMENU_PARENT::onInit();
  postDeferredCallback(DC_MENU_INIT);
  return 1;
}

int XuiMenu::setXuiParam(int _xuihandle, int xmlattrid, const char *name, const char *value) {
  if (xuihandle == _xuihandle) {
    switch (xmlattrid) {
      case MENU_MENUGROUP: setMenuGroup(value); return 1;
      case MENU_MENU: setMenu(value); return 1;
      case MENU_NORMALID: setNormalId(value); return 1;
      case MENU_DOWNID: setDownId(value); return 1;
      case MENU_HOVERID: setHoverId(value); return 1;
      case MENU_NEXT: next = value; return 1;
      case MENU_PREV: prev = value; return 1;
    }
  }
  return XUIMENU_PARENT::setXuiParam(_xuihandle, xmlattrid, name, value);
}

void XuiMenu::setMenu(const char *m) {
  menuid = m;
  return;
}

void XuiMenu::setMenuGroup(const char *mg) {
  menugroup = mg;
  return;
}

void XuiMenu::setNormalId(const char *id) {
  normalid = id;
  updateObjects();
}

void XuiMenu::setDownId(const char *id) {
  downid = id;
  updateObjects();
}

void XuiMenu::setHoverId(const char *id) {
  hoverid = id;
  updateObjects();
}

int XuiMenu::onLeftButtonDown(int x, int y) {
  XUIMENU_PARENT::onLeftButtonDown(x, y);
  if (!disablespawn)
    spawnMenu(); 
  return 1;
}

void XuiMenu::timerCallback(int c) {
  if (c == MENU_TIMER_CHECK) {
    timerCheck();
  } else if (c == MENU_TIMER_POPKBDLOCK) {
    kbdlocktimer = 0;
    killTimer(MENU_TIMER_POPKBDLOCK);
    WASABI_API_WND->popKeyboardLock();
  }
  else XUIMENU_PARENT::timerCallback(c);
}

void XuiMenu::timerCheck() {
  POINT pt;
  Std::getMousePos((long*)&pt.x, (long*)&pt.y);
  if (pt.x == orig_x && pt.y == orig_y) return;
  nextinchain = NULL;
  RootWnd *w = WASABI_API_WND->rootWndFromPoint(&pt);
  if (w != NULL) {
    XuiMenu *x = static_cast<XuiMenu *>(w->getInterface(xuiMenuGuid));
    switchToMenu(x);
  }
}

void XuiMenu::switchToMenu(XuiMenu *x) {
  if (x != NULL && x != this && x != nextinchain) {
    if (STRCASEEQLSAFE(x->getMenuGroup(), getMenuGroup())) {
      killTimer(MENU_TIMER_CHECK); 
      timerset = 0; 
      nextinchain = x;
      stopKbdHook();
      cancelMenu();
    }
  }
}

void XuiMenu::spawnMenu(int monitor) {
  
  Std::getMousePos(&orig_x, &orig_y);

  isspawned = 1;
  onOpenMenu();

  if (monitor) { setTimer(MENU_TIMER_CHECK, MENU_TIMER_DELAY); timerset = 1; startKbdHook(); }

  String actionstr = StringPrintf("MENU:%s", menuid);

  svc_action *act = ActionEnum(actionstr).getNext();
  if (act) {
    RECT r;
    getClientRect(&r);
    clientToScreen(&r);
    act->onAction(actionstr, NULL, r.left, r.bottom, NULL, 0, this);
    SvcEnum::release(act);
  }

  if (monitor) {
    if (timerset) { killTimer(MENU_TIMER_CHECK); timerset = 0; }
    stopKbdHook();
  }
  disablespawn = 1;
  if (nextinchain) {
    postDeferredCallback(DC_MENU_CHAIN);
  } 
  postDeferredCallback(DC_MENU_REENABLE);

  isspawned = 0;
  onCloseMenu();
}

int XuiMenu::onDeferredCallback(int p1, int p2) {
  if (p1 == DC_MENU_CHAIN) {
    XuiMenu *x = nextinchain;
    nextinchain = NULL;
    x->spawnMenu();
  } else if (p1 == DC_MENU_REENABLE) {
    disablespawn = 0;
  } else if (p1 == DC_MENU_INIT) {
    updateObjects();
  } else if (p1 == DC_MENU_NEXT) {
    _nextMenu();
  } else if (p1 == DC_MENU_PREV) {
    _previousMenu();
  } else if (p1 == DC_MENU_OPENACTION) {
    spawnMenu(); 
    WASABI_API_WND->kbdReset();
  } else return XUIMENU_PARENT::onDeferredCallback(p1, p2);
  return 1;
}

void XuiMenu::cancelMenu() {
  #ifdef WIN32

  PostMessage(gethWnd(), WM_LBUTTONDOWN, 0, 0xdeadc0de);
  PostMessage(gethWnd(), WM_LBUTTONUP, 0, 0xdeadc0de);

  #else
  
  #error port me! you should close that menu which is in its own message pump now (oh and btw, you don't know shit about the menu itself because it's spawned by an action). have fun!
  
  #endif
}

void XuiMenu::onOpenMenu() {
  updateObjects();
  script_onOpenMenu();
}

void XuiMenu::onCloseMenu() {
  updateObjects();
  WASABI_API_WND->kbdReset();
  script_onCloseMenu();
}

void XuiMenu::updateObjects() {
  normal = findObject(normalid);
  down = findObject(downid);
  hover = findObject(hoverid);
  if (normal) normal->guiobject_getRootWnd()->setVisible(!isspawned);
  if (down) down->guiobject_getRootWnd()->setVisible(isspawned);
  if (hover) hover->guiobject_getRootWnd()->setVisible(!isspawned && inarea);
}

void XuiMenu::onEnterArea() {
  inarea = 1;
  updateObjects();
}

void XuiMenu::onLeaveArea() {
  inarea = 0;
  updateObjects();
}

int XuiMenu::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source) {
  if (STRCASEEQL(action, "open")) { openAction(); }
  //else if (STRCASEEQL(action, "preopen")) { inarea = 1; updateObjects(); startKbdHook(); }
  else if (STRCASEEQL(action, "close")) cancelMenu();
  else return XUIMENU_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, source);
  return 1;
}

LRESULT CALLBACK xuimenu_KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code >= 0 && xuimenu_hookingMenu != NULL) {
    if (code != HC_NOREMOVE && !(lParam & (1 << 31))) {
      switch (wParam) {
        case VK_LEFT: xuimenu_hookingMenu->onTrappedLeft(); break;
        case VK_RIGHT: xuimenu_hookingMenu->onTrappedRight(); break;
        //case VK_ESCAPE: DebugString("Escape trapped!\n"); break;
      }
    }
  }
  if (hhook) return CallNextHookEx(hhook, code, wParam, lParam);
  return 1;
} 

LRESULT CALLBACK xuimenu_msgProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code >= 0 && xuimenu_hookingMenu != NULL) {
    MSG *msg = (MSG *)lParam;
    if (msg->message == WM_MENUSELECT) {
      xuimenu_hookingMenu->onMenuSelect(msg->hwnd, (HMENU)msg->lParam, LOWORD(msg->wParam), HIWORD(msg->wParam));
    } 
  }
  if (hhook_menuselect) return CallNextHookEx(hhook_menuselect, code, wParam, lParam);
  return 1;
} 

void XuiMenu::startKbdHook() {
  if (kbdhook) return;
  if (kbdlocktimer) { killTimer(MENU_TIMER_POPKBDLOCK); kbdlocktimer = 0; }
  else WASABI_API_WND->pushKeyboardLock();
  xuimenu_hookingMenu = this;
  if (hhook == NULL) {
    hhook = SetWindowsHookEx(WH_KEYBOARD, xuimenu_KeyboardProc, WASABI_API_APP->main_gethInstance(), 0);
    hhook_menuselect = SetWindowsHookEx(WH_MSGFILTER, xuimenu_msgProc, WASABI_API_APP->main_gethInstance(), 0);
    hookusercount = 1;
  } else hookusercount++;
  kbdhook = 1;
}

void XuiMenu::stopKbdHook() {
  if (!kbdhook) return;
  if (nextinchain) {
    if (!kbdlocktimer) {
      kbdlocktimer = 1;
      setTimer(MENU_TIMER_POPKBDLOCK, 500);
    }
  } else WASABI_API_WND->popKeyboardLock();
  if (xuimenu_hookingMenu == this) xuimenu_hookingMenu = NULL;
  first_hmenu = NULL;
  submenu_isselected = 0;
  submenu_selectedbymouse = 0;
  submenu_selected = NULL;
  cur_hmenu = NULL;
  menu_parent = NULL;
  if (--hookusercount == 0) {
    UnhookWindowsHookEx(hhook_menuselect);
    UnhookWindowsHookEx(hhook);
    hhook = NULL;
    hhook_menuselect = NULL;
  }
  kbdhook = 0;
}

void XuiMenu::onMenuSelect(HWND hwnd, HMENU menu, int item, int flags) {
  if (first_hmenu == NULL) first_hmenu = menu;
  cur_hmenu = menu;
  menu_parent = hwnd;

//  DebugString("Menu Item Selected! HMENU = %X, item = %d, flags = %d, submenu = %X\n", menu, item, flags, flags & MF_POPUP);
  
  if (flags & MF_POPUP) { submenu_isselected = 1; submenu_selected = GetSubMenu(menu, item); submenu_selectedbymouse = flags & MF_MOUSESELECT; }
  else { submenu_isselected = 0; submenu_selected = NULL; submenu_selectedbymouse = 0; }
}

void XuiMenu::onTrappedLeft() {
  if (cur_hmenu == first_hmenu) 
    xuimenu_hookingMenu->previousMenu();
}

void XuiMenu::onTrappedRight() {
  if (!submenu_isselected) 
    nextMenu(); 
  else if (submenu_selectedbymouse)
    PostMessage(menu_parent, WM_KEYDOWN, VK_DOWN, 0);
}


void XuiMenu::openAction() {
  postDeferredCallback(DC_MENU_OPENACTION);
}

void XuiMenu::previousMenu() {
  postDeferredCallback(DC_MENU_PREV);
}

void XuiMenu::nextMenu() {
  postDeferredCallback(DC_MENU_NEXT);
}

void XuiMenu::_nextMenu() {
  GuiObject *o = findObject(next);
  if (o) {
    XuiMenu *menu = static_cast<XuiMenu*>(o->guiobject_getScriptObject()->vcpu_getInterface(xuiMenuGuid));
    if (menu) switchToMenu(menu);
  }
}

void XuiMenu::_previousMenu() {
  GuiObject *o = findObject(prev);
  if (o) {
    XuiMenu *menu = static_cast<XuiMenu*>(o->guiobject_getScriptObject()->vcpu_getInterface(xuiMenuGuid));
    if (menu) switchToMenu(menu);
  }
}

