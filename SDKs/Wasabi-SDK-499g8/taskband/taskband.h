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
#ifndef _TASKBANDWAC_H
#define _TASKBANDWAC_H

#define DB_CLASS_NAME "Winamp3_TaskBand"

#include "../studio/wac.h"
#include "../bfc/basewnd.h"
#include "../studio/runlevelcb.h"
#include "../studio/skincb.h"

// {484F48B8-AC60-4e80-8419-222CC32F7A67}
static const GUID taskbandGuid = 
{ 0x484f48b8, 0xac60, 0x4e80, { 0x84, 0x19, 0x22, 0x2c, 0xc3, 0x2f, 0x7a, 0x67 } };

class MyWnd : public BaseWnd {
  public :
    MyWnd() {}
    virtual ~MyWnd() {}
};

class WACTaskband : public WAComponentClient, public RunlevelCallbackI, public SkinCallbackI {
public:
  WACTaskband();

  virtual GUID getGUID();

  virtual void onCreate();
  virtual void onDestroy();

  virtual void onSkinLoaded();
  virtual const char *getFindWndClass() { return findclass; }
  virtual void setFoundWnd(HWND w) { foundwnd = w; }

  virtual void runlevelcb_onShutdown();
  virtual int skincb_onUnloading();

private:

  void createParent();
  void createGroup();
  void destroyGroup();
  HWND myFindWindow(const char *classname);

  RootWnd *group;
  MyWnd *myparent;
  HWND foundwnd;
  String findclass;

};

extern WACTaskband *taskbandwac;

#endif
