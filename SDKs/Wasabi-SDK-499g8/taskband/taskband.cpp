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
#include "taskband.h"
#include "resource.h"
#include "../common/guiobjwnd.h"
#include "../studio/api.h"
#include "../bfc/rootwnd.h"

static WACTaskband taskband;
WAComponentClient *the = &taskband;
WACTaskband *taskbandwac = &taskband;

int disallow_select_default=0;

// Our component constructor, register various things.
// REMEMBER, accessing the api pointer here is forbiden, use onCreate for that.
WACTaskband::WACTaskband() {
  
  group = NULL;
  myparent = NULL;

  // register the taskband window xml
  registerSkinFile("xml/taskband.xml"); 
}

GUID WACTaskband::getGUID() {
  return taskbandGuid;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
  WACTaskband *me = (WACTaskband *)lParam;
  char t[256];
  GetClassName(hwnd, t, 255);
  t[255]=0;
  if (STRCASEEQLSAFE(t, me->getFindWndClass())) {
    me->setFoundWnd(hwnd);
    return FALSE;
  }
  return TRUE;
}
 
HWND WACTaskband::myFindWindow(const char *classname) {
  findclass = classname;
  HWND tray = FindWindow("Shell_TrayWnd", NULL);
  if (tray == NULL) return NULL;
  EnumChildWindows(tray, EnumWindowsProc, (int)this);
  HWND w = foundwnd;
  foundwnd = NULL;
  return w;
}

void WACTaskband::runlevelcb_onShutdown() {
  destroyGroup();
  delete myparent;
  myparent = NULL;
}


void WACTaskband::onCreate() {
  api->syscb_registerCallback(static_cast<RunlevelCallbackI *>(this));
  api->syscb_registerCallback(static_cast<SkinCallbackI *>(this));
  HWND tb = myFindWindow(DB_CLASS_NAME);
  if (tb == NULL) return;
  createParent();
}

void WACTaskband::onDestroy() {
  api->syscb_registerCallback(static_cast<SkinCallbackI *>(this));
  api->syscb_deregisterCallback(static_cast<SkinCallbackI *>(this));
}

void WACTaskband::createParent() {
  if (myparent != NULL) return;
  myparent = new MyWnd();
  myparent->setStartHidden(1);
  myparent->init(gethInstance(), NULL, 1);
  RECT r={0,0,0,0};
  myparent->resize(&r);
  myparent->setVisible(1);
}

void WACTaskband::onSkinLoaded() {
  createParent();
  createGroup();
}

void WACTaskband::destroyGroup() {
  if (group != NULL) {
    api->group_destroy(group);
    group = NULL;
  }
}

void WACTaskband::createGroup() {
  if (group != NULL) destroyGroup();
  group = api->group_create_layout("wasabi.taskband.group");
  group->setStartHidden(1);
  group->setParent(myparent);
  group->init(myparent, 1);
  HWND tb = myFindWindow(DB_CLASS_NAME);
  SetParent(group->gethWnd(), tb);
  RECT r;
  GetClientRect(tb, &r);
  group->resize(0, 0, r.right-r.left, r.bottom-r.left);
  group->setVisible(1);
}

int WACTaskband::skincb_onUnloading() {
  destroyGroup();
  return 0;
}

