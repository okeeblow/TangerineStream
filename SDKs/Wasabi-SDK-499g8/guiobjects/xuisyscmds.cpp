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

#include "xuisyscmds.h"
#include "../common/popup.h"
#include "../studio/layout.h"

char SysCmdsXuiObjectStr[]="SysCmds";
char SysCmdsXuiSvcName[]="SysCmds Service";

SysCmds::SysCmds() {
}

SysCmds::~SysCmds() {
  getGuiObject()->guiobject_removeAppCmds(this);
}

int SysCmds::onInit() {
  SYSCMDS_PARENT::onInit();
  appcmds_addCmd("{system}", 0, AppCmds::SIDE_LEFT);
  getGuiObject()->guiobject_addAppCmds(this);
  return 1;
}

void SysCmds::appcmds_onCommand(int id, const RECT *buttonRect, int which_btn) {
  if (id == 0) {
    Layout *l = getGuiObject()->guiobject_getParentLayout();
    PopupMenu pop(this);
    pop.addCommand("Close window", ACTION_CLOSE_WINDOW);
    pop.addSeparator();
    PopupMenu *scalemenu = new PopupMenu();
    scalemenu->addCommand("50%", ACTION_SCALE_50);
    scalemenu->addCommand("75%", ACTION_SCALE_75, FALSE, FALSE);
    scalemenu->addCommand("100%", ACTION_SCALE_100);
    scalemenu->addCommand("125%", ACTION_SCALE_125);
    scalemenu->addCommand("150%", ACTION_SCALE_150);
    scalemenu->addCommand("200%", ACTION_SCALE_200);
    scalemenu->addCommand("400%", ACTION_SCALE_400, FALSE, FALSE);
    pop.addSubMenu(scalemenu, "Scale");
    if (BaseWnd::isDesktopAlphaAvailable()) {
      PopupMenu *opaqmenu = new PopupMenu();
      opaqmenu->addCommand("100%", ACTION_ALPHA_100, FALSE, FALSE);
      opaqmenu->addCommand("90%", ACTION_ALPHA_90, FALSE, FALSE);
      opaqmenu->addCommand("80%", ACTION_ALPHA_80, FALSE, FALSE);
      opaqmenu->addCommand("70%", ACTION_ALPHA_70, FALSE, FALSE);
      opaqmenu->addCommand("60%", ACTION_ALPHA_60, FALSE, FALSE);
      opaqmenu->addCommand("50%", ACTION_ALPHA_50, FALSE, FALSE);
      opaqmenu->addCommand("40%", ACTION_ALPHA_40, FALSE, FALSE);
      opaqmenu->addCommand("30%", ACTION_ALPHA_30, FALSE, FALSE);
      opaqmenu->addCommand("20%", ACTION_ALPHA_20, FALSE, FALSE);
      opaqmenu->addCommand("10%", ACTION_ALPHA_10, FALSE, FALSE);
      pop.addSubMenu(opaqmenu, "Opacity");
    }
    int r = pop.popAtXY(buttonRect->left, buttonRect->bottom);
    if (r < 0) return;
    l->onActionNotify(r, 0);
  }
}
