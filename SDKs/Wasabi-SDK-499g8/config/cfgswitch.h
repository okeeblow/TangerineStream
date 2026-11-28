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
#ifndef _CFGSWITCH_H
#define _CFGSWITCH_H

#include "../common/framewnd.h"
#include "compcfgitem.h"
#include "../studio/skincb.h"

class CfgWnd;	// cfgwnd.h
class TreeWnd;	// see ../common/treewnd.h
class TreeItem; // see ../common/treewnd.h
class ButtonWnd;

#define CFGSWITCHER_PARENT FrameWnd
class CfgSwitcher : public CFGSWITCHER_PARENT, public SkinCallbackI {
public:
  CfgSwitcher();
  virtual ~CfgSwitcher();

  static const char *getWindowTypeName() { return "Configurification Registry"; }
  static GUID getWindowTypeGuid() {
    // {395F5A09-E187-4e5d-87BC-1526B53ADEC8}
    static const GUID ret = 
    { 0x395f5a09, 0xe187, 0x4e5d, { 0x87, 0xbc, 0x15, 0x26, 0xb5, 0x3a, 0xde, 0xc8 } };
    return ret;
  }
//  static void setIconBitmaps(ButtonWnd *button);

  virtual int onInit();

  virtual int skincb_onUnloading();
  virtual int skincb_onGuiLoaded();

private:
  void insertItem(CfgItem *item);

  TreeWnd *itemlist;
  CfgWnd *cfgwnd;
  TreeItem *backup_item;
  ComponentCfgItem compcfg;
};

#endif
