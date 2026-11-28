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
#include "../bfc/std.h"
#include "grouptips.h"
#include "../studio/api.h"
#include "../bfc/rootwnd.h"
#include "../common/script/c_script/c_group.h"
#include "../common/script/c_script/c_text.h"
#include "../common/script/scriptguid.h"
#include "../common/script/guiobject.h"

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(GroupTips_Svc);
DECLARE_SERVICETMULTI(svc_toolTipsRenderer, GroupTips);
END_SERVICES(GroupTips_Svc, _GroupTips_Svc);

#ifdef _X86_
extern "C" { int _link_GroupTipsSvc; }
#else
extern "C" { int __link_GroupTipsSvc; }
#endif

#endif

GroupTips::GroupTips() {
  tipwnd = NULL;
}

GroupTips::~GroupTips() {
  if (tipwnd) 
    WASABI_API_SKIN->group_destroy(tipwnd);
}

int GroupTips::spawnTooltip(const char *text) {

  int x, y;
  Std::getMousePos(&x, &y);

  RootWnd *wnd = WASABI_API_SKIN->group_create_layout("wasabi.tooltip.group");

  if (wnd) {
    wnd->setStartHidden(1);
    wnd->setParent(WASABI_API_WND->main_getRootWnd());
    wnd->init(WASABI_API_WND->main_getRootWnd(), TRUE);
    wnd->getGuiObject()->guiobject_onStartup();

    RECT r;
    wnd->getClientRect(&r);
    int w = r.right - r.left;
    int h = r.bottom - r.top;

    y -= h; // move tip above mouse by default

    POINT pt={x, y};
    RECT vpr;

    Std::getViewport(&vpr, &pt, NULL, NULL);

    if (x + w > vpr.right) x -= vpr.right - w;
    if (x < vpr.left) x = vpr.left;
    if (x + w > vpr.right) { w = vpr.right-vpr.left; x = 0; }
    if (y + h > vpr.bottom) y -= vpr.bottom - w;
    if (y < vpr.top) y = vpr.top;
    if (y + h > vpr.bottom) { h = vpr.bottom-vpr.top; y = 0; }

    wnd->resize(x, y, w, h);

    ScriptObject *group = static_cast<ScriptObject *>(wnd->getInterface(scriptObjectGuid));
    if (group) {
      C_Group *g = new C_Group(group);
      ScriptObject *txt = g->getObject("tooltip.text");
      if (txt) {
        C_Text *t = new C_Text(txt);
        t->setText(text);
        delete t;
      }
      delete g;
    }

    wnd->setVisible(1);

    tipwnd = wnd;
  }
  return 1;
}

