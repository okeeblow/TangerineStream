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
// PopupMenu NONPORTABLE, NewPopupMenu portable
#ifndef _POPUP_H
#define _POPUP_H

#include <wasabicfg.h>

//#define WANT_NEW_POPUPMENU

class PopupMenu;

class PopupMenuCallback {
public:
  virtual PopupMenu *popupMenuCallback(PopupMenu *parent, int param)=0; // returns the new popupmenu to be displayed
};

enum { POPUP_ANCHOR_UL, POPUP_ANCHOR_LL, POPUP_ANCHOR_UR, POPUP_ANCHOR_LR };

#ifndef WASABI_WANT_FF_POPUP

#ifdef WIN32
#define OSMENUHANDLE HMENU
#else
port me!
#endif

class PopupMenuEntry {
  public:
    PopupMenuEntry(const char *txt, int cmd, PopupMenu *_submenu, int _checked, int _disabled, PopupMenuCallback *cb=NULL, int cbparam=0) : text(txt), command(cmd), submenu(_submenu), checked(_checked), disabled(_disabled), callback(cb), callbackparam(cbparam) {} // txt = null for separators and menus, cmd = -1 for separators and submenus, if submenu == null && cmd == -1 and text == NULL then it's a separator
    virtual ~PopupMenuEntry() {}
    int getCommand() { return command; }
    const char *getText() { return text; }
    PopupMenu *getSubmenu() { return submenu; }
    int getChecked() { return checked; }
    int getDisabled() { return disabled; }
    int isSeparator() { return command == -1 && submenu == NULL && text == NULL; }
    int isSubmenu() { return (command == -1 && submenu != NULL) || callback; }
    void setChecked(int check) { checked = check; }
    void setDisabled(int disable) { disabled = disable; }
    PopupMenuCallback *getCallback() { return callback; }
    int getCallbackParam() { return callbackparam; }
  private:
    String text;
    int command;
    PopupMenu *submenu;
    int checked;
    int disabled;
    PopupMenuCallback *callback;
    int callbackparam;
};

class SortMenuEntries {
public:
  static int compareItem(PopupMenuEntry *p1, PopupMenuEntry *p2) {
   if (p1->getCommand() < p2->getCommand()) return -1;
   if (p1->getCommand() > p2->getCommand()) return 1;
   return 0;
  }
  static int compareAttrib(const char *attrib, PopupMenuEntry *item) {
   int c = *(int *)attrib;
   if (c < item->getCommand()) return -1;
   if (c > item->getCommand()) return 1;
   return 0;
  }
};


class COMEXP PopupMenu {
  public:
    PopupMenu();
    PopupMenu(RootWnd *parent);
    PopupMenu(PopupMenu *parent);
    ~PopupMenu();

    virtual void addSubMenu(PopupMenu *menu, const char *text, int disabled=FALSE);
    virtual void addSubMenuCallback(const char *text, PopupMenuCallback *cb, int param);
    virtual void addCommand(const char *text, int command, int checked=0, int disabled=0, int addpos=-1);
    virtual void addSeparator(int addpos=-1);
    virtual void checkCommand(int cmd, int check);
    virtual void disableCommand(int cmd, int disable);
    virtual int popAtXY(int x, int y);
    virtual int popAnchored(int type = POPUP_ANCHOR_LL);	// dropped off the sourceWnd given above
    virtual int popAtMouse();
    virtual int getNumCommands();
    virtual const char *getCommandText(int command);
    OSMENUHANDLE getOSMenuHandle();
    virtual void abort();

  private:
    void rebuildMenu();
    void invalidateMenu(); // no virtual please (it's called in the destructor)
    RootWnd *getParent() { return parent; }

    PtrList<PopupMenuEntry> entries;
    PtrListQuickSorted<PopupMenuEntry, SortMenuEntries> sortedentries;
    OSMENUHANDLE hmenu;
    RootWnd *parent;
};

#else // WASABI_WANT_FF_POPUP

#include "../bfc/basewnd.h"
#include "../bfc/ptrlist.h"
#include "../bfc/autobitmap.h"
#include "../studio/skincb.h" 

#define POPUP_TIMERID 1171

class PopupMenu;
class ButtonWnd;
class MenuButtonSurface;

class COMEXP _PopupMenu {
  public:
    virtual ~_PopupMenu() {}

    virtual void addSubMenu(PopupMenu *menu, const char *text, int disabled=FALSE)=0;
    virtual void addSubMenuImage(PopupMenu *menu, const char *bitmap, const char *pushedbitmap=0, const char *highlightbitmap=0)=0;
    virtual void addSubMenuCallback(const char *text, PopupMenuCallback *cb, int param)=0;

    virtual void addCommand(const char *text, int command, int checked=0, int disabled=0, int addpos=-1)=0;
    virtual void addCommandImage(const char *bitmap, const char *pushedbitmap, const char *highlightbitmap, int command, int checked, int disabled, int addpos=-1)=0;
    virtual void addSeparator(int addpos=-1)=0;

    virtual void checkCommand(int cmd, int check)=0;
    virtual void disableCommand(int cmd, int disable)=0;

    virtual int popAtXY(int x, int y)=0;
    virtual int popAnchored(int type = POPUP_ANCHOR_LL)=0;	// dropped off the sourceWnd given above
    virtual int popAtMouse()=0;
    virtual void showAtXY(int x, int y, int *rc, int revside=0, int parentW=0)=0;
    virtual int getNumCommands()=0;
    virtual const char *getCommandText(int command)=0;
    virtual void onPostPop(int result)=0;
    virtual void selectFirst()=0;
};

#ifndef WANT_NEW_POPUPMENU

#define POPUPMENU_PARENT BaseWnd

class COMEXP PopupMenu : public _PopupMenu, public POPUPMENU_PARENT, public SkinCallbackI {
public:
  PopupMenu(RootWnd *sourceWnd);
  PopupMenu(HWND sourceWnd);
  PopupMenu(PopupMenu *sourceWnd);
  PopupMenu();
  virtual ~PopupMenu();

  void addSubMenu(PopupMenu *menu, const char *text, int disabled=FALSE);
  void addSubMenuImage(PopupMenu *menu, const char *bitmap, const char *pushedbitmap=0, const char *highlightbitmap=0);
  void addSubMenuCallback(const char *text, PopupMenuCallback *cb, int param);

  void addCommand(const char *text, int command, int checked=0, int disabled=0, int addpos=-1);
  void addCommandImage(const char *bitmap, const char *pushedbitmap, const char *highlightbitmap, int command, int checked, int disabled, int addpos=-1);
  void addSeparator(int addpos=-1);

  void checkCommand(int cmd, int check);
  void disableCommand(int cmd, int disable);

  int popAtXY(int x, int y);
  int popAnchored(int type = POPUP_ANCHOR_LL);	// dropped off the sourceWnd given above
  int popAtMouse();
  int getNumCommands();

  virtual int bypassModal() { return 1; }

  const char *getCommandText(int command);

  virtual int onPaint(Canvas *canvas);

  virtual int childNotify(RootWnd *child, int msg, int param1=0, int param2=0);

  virtual int onLeftButtonDown(int x, int y);
  virtual int onRightButtonDown(int x, int y);
  virtual int onMouseMove(int x, int y);	// only called when mouse captured
  virtual int onLeftButtonUp(int x, int y);
  virtual int onRightButtonUp(int x, int y);
  virtual void timerCallback(int id);

  virtual int onKillFocus();

  virtual int skincb_onCheckPreventSwitch(const char *skinname);

  virtual int onKeyDown(int code);

  virtual void navigate(int p, int f=0); // 1 = next, -1 = previous, 0 = self (open submenu or run action)

  virtual void onSetVisible(int p);
  virtual void selectFirst();
  virtual int onSysKeyDown(int code, int d);

  virtual void abort();

  virtual void setFriendlyId(const char *id);
  virtual MenuButtonSurface *getNextFriend() { return chainmenu; }

protected:
  virtual int onInit();

  void invalidateItem(int i);
  String translateButtonText(const char *text);

  virtual void onPostPop(int result) {}

  // used internally, as well as by parent Popups.
  void showAtXY(int x, int y, int *rc, int revside=0, int parentW=0);
  void hide();    
  int isMine(int x, int y);
private:
  int getWhichItem(POINT &p);
  void onButtonUp(int wb, int x, int y);
  void onButtonDown(int wb, int x, int y);
  void resetTimer(int p);
  void initMenuCallback(int item);

  int bdown;
  int lastitem;
  int rcode;
  int *rcp;
  typedef struct {
    int cmd;
    ButtonWnd *butt;
    PopupMenu *menu;
    PopupMenuCallback *cb;
    int issep;
    int cbparam;
  } ItemT;
  PtrList<ItemT> items;  
  RootWnd *parentRootWnd;
  HWND parentWnd;
  AutoSkinBitmap tex, ful, fur, fll, flr, fl, fr, ft, fb, sr, sl, sc;
  int openmenuid;
  int timerset;
  int timeritem;
  int popupdelay;
  int reverse_side;
  RootWnd *init_with;
  int disable_autopop;
  int kbdhooked;
  int toplevelmenu;
  POINT lastxy;
  int keyctrl;
  String friendid;
  MenuButtonSurface *chainmenu;

#ifdef WASABI_COMPILE_SKIN
  String switchskinto;
#endif
};

#else

#include "guiobjwnd.h"
#include "script/c_script/c_grouplist.h"
#include "../bfc/popexitcb.h"
#include "../bfc/notifmsg.h"

#define POPUPMENU_PARENT GuiObjectWnd

enum {
  POPUPITEM_TYPE_TEXT=0,
  POPUPITEM_TYPE_IMAGE,
  POPUPITEM_TYPE_SEPARATOR,
};

typedef struct {
  int type;
  int cmd;
  PopupMenu *menu;
  PopupMenuCallback *cb;
  String txt;
  int checked;
  int disabled;
  int cbparam;
} ItemT;

class COMEXP PopupMenu : public _PopupMenu, public POPUPMENU_PARENT, public PopupExitCallbackI {
  public:
    PopupMenu(RootWnd *sourceWnd);
//    PopupMenu(HWND sourceWnd);
    PopupMenu(PopupMenu *sourceWnd);
    PopupMenu();
    virtual ~PopupMenu();

    virtual void addSubMenu(PopupMenu *menu, const char *text, int disabled=FALSE);
    virtual void addSubMenuImage(PopupMenu *menu, const char *bitmap, const char *pushedbitmap=0, const char *highlightbitmap=0) {}
    virtual void addSubMenuCallback(const char *text, PopupMenuCallback *cb, int param);

    virtual void addCommand(const char *text, int command, int checked=0, int disabled=0, int addpos=-1);
    virtual void addCommandImage(const char *bitmap, const char *pushedbitmap, const char *highlightbitmap, int command, int checked, int disabled, int addpos=-1) {}
    virtual void addSeparator(int addpos=-1) {}

    virtual void checkCommand(int cmd, int check) {}
    virtual void disableCommand(int cmd, int disable) {}

    virtual int popAtXY(int x, int y);
    virtual int popAnchored(int type = POPUP_ANCHOR_LL);
    virtual int popAtMouse();
    virtual void showAtXY(int x, int y, int *rc, int revside=0, int parentW=0);
    virtual int getNumCommands() { return 0;}
    virtual const char *getCommandText(int command) { return NULL; }
    virtual void onPostPop(int result) {}

    virtual int onInit();

    virtual void onNewContent();

    virtual int popupexitcb_onExitPopup();
    virtual Dependent *popupexit_getDependencyPtr() { return getDependencyPtr(); }

  private:

    virtual int bypassModal() { return 1; }
    String translateButtonText(const char *text);
    void fillContent();
    void addItem(ItemT *item);
    void initMenuCallback(int item);

    void myInit();
    int reverse_side;
    int rcode;

    PtrList<ItemT> items;  
    int menuchecks;
    int submenus;
    C_GroupList *c_grouplist;
    int totalheight, totalwidth;
};

#endif
#endif // WASABI_WANT_FF_POPUP
#endif
