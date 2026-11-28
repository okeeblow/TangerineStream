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
#ifndef _XUIMENU_H
#define _XUIMENU_H

#include "common/guiobjwnd.h"

/*<?<autoheader/>*/
#include "xuimenuso.h"
/*?>*/

#define XUIMENU_PARENT XuiMenuScriptObject

// {A0211C57-DCED-45ae-AEA6-56014B5898E8}
static const GUID xuiMenuGuid = 
{ 0xa0211c57, 0xdced, 0x45ae, { 0xae, 0xa6, 0x56, 0x1, 0x4b, 0x58, 0x98, 0xe8 } };

/*<?<classdecl name="XuiMenu" factory="ScriptObject" />*/
class XuiMenu : public XuiMenuScriptObject {
/*?>*/
  public:
    friend LRESULT CALLBACK xuimenu_KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
    friend LRESULT CALLBACK xuimenu_msgProc(int code, WPARAM wParam, LPARAM lParam);

    XuiMenu();
    virtual ~XuiMenu();

    virtual int onInit();
    virtual int onLeftButtonDown(int x, int y);
    int setXuiParam(int _xuihandle, int xmlattrid, const char *name, const char *value);
    virtual void timerCallback(int c);
    virtual int onDeferredCallback(int p1, int p2);

    SCRIPT void setMenuGroup(const char *mg);
    SCRIPT const char *getMenuGroup() { return menugroup; }

    SCRIPT void setMenu(const char *m);
    SCRIPT const char *getMenu() { return menuid; }

    SCRIPT void spawnMenu(int monitor = 1);
    SCRIPT void cancelMenu();

    SCRIPT void setNormalId(const char *id);
    SCRIPT void setDownId(const char *id);
    SCRIPT void setHoverId(const char *id);

    SCRIPT EVENT virtual void onOpenMenu();
    SCRIPT EVENT virtual void onCloseMenu();

    virtual void onEnterArea();
    virtual void onLeaveArea();
    virtual int onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source);

    SCRIPT void nextMenu();
    SCRIPT void previousMenu();
    
    enum {
      MENU_MENU = 0,
      MENU_MENUGROUP,
      MENU_NORMALID,
      MENU_DOWNID,
      MENU_HOVERID,
      MENU_NEXT,
      MENU_PREV,
    };

  private:
    void onMenuSelect(HWND hwnd, HMENU menu, int menuitem, int flags);
    void startKbdHook();
    void stopKbdHook();
    void updateObjects();
    void timerCheck();
    void switchToMenu(XuiMenu *menu);
    void _nextMenu();
    void _previousMenu();
    void openAction();
    void onTrappedLeft();
    void onTrappedRight();
    String menugroup;
    String menuid;
    int xuihandle;
    XuiMenu *nextinchain;
    int timerset;
    int disablespawn;
    GuiObject *normal;
    GuiObject *down;
    GuiObject *hover;
    String next, prev;
    int isspawned;
    String normalid, downid, hoverid;
    int inarea;
    int kbdhook;
    int orig_x, orig_y;
    int kbdlocktimer;
    int submenu_isselected;
    int submenu_selectedbymouse;
    HMENU submenu_selected;
    HWND menu_parent;
    HMENU first_hmenu;
    HMENU cur_hmenu;
};

// -----------------------------------------------------------------------
extern char MenuXuiObjectStr[];
extern char MenuXuiSvcName[];
class MenuXuiSvc : public XuiObjectSvc<XuiMenu, MenuXuiObjectStr, MenuXuiSvcName> {};

#endif
