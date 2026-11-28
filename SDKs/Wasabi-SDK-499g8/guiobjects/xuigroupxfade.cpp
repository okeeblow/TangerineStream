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
#include "xuigroupxfade.h"


#ifndef _WASABIRUNTIME

BEGIN_SERVICES(GroupXFade_Svc);
DECLARE_SERVICE(XuiObjectCreator<GroupXFadeXuiSvc>);
END_SERVICES(GroupXFade_Svc, _GroupXFade_Svc);

#ifdef _X86_
extern "C" { int _link_GroupXFadeXuiSvc; }
#else
extern "C" { int __link_GroupXFadeXuiSvc; }
#endif

#endif

GroupXFade::GroupXFade() {
  child[0] = child[1] = NULL;
  curchild = 0;
  myxuihandle = newXuiHandle();
  double speed = 0.25;
  
  addParam(myxuihandle, "group", GROUPXFADE_SETGROUP, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "groupid", GROUPXFADE_SETGROUP, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "speed", GROUPXFADE_SETSPEED, XUI_ATTRIBUTE_IMPLIED);
}

GroupXFade::~GroupXFade() {
  if (child[0]) {
    RootWnd *w = child[0]->guiobject_getRootWnd();
    if (w) WASABI_API_SKIN->group_destroy(w);
  }
  if (child[1]) {
    RootWnd *w = child[1]->guiobject_getRootWnd();
    if (w) WASABI_API_SKIN->group_destroy(w);
  }
}

int GroupXFade::onInit() {
  GROUPXFADE_PARENT::onInit();
  return 1;
}

int GroupXFade::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return GROUPXFADE_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch(xmlattributeid) {
    case GROUPXFADE_SETGROUP:
      setNewGroup(value);
      return 1;
    case GROUPXFADE_SETSPEED:
      speed = ATOF(value);
      return 1;
    default:
      return 0;
  }
  return 1;
}

int GroupXFade::onResize() {
  GROUPXFADE_PARENT::onResize();
  RECT r;
  getClientRect(&r);
  if (child[curchild]) 
    child[curchild]->guiobject_getRootWnd()->resize(r.left, r.top, r.right-r.left, r.bottom-r.top);
  int nextchild = curchild == 1 ? 0 : 1;
  if (child[nextchild]) 
    child[nextchild]->guiobject_getRootWnd()->resize(r.left, r.top, r.right-r.left, r.bottom-r.top);
  return 1;
}

void GroupXFade::setNewGroup(const char *grp) {
  if (child[curchild] && id[curchild].iscaseequal(grp)) return;
  if (child[curchild]) {
    child[curchild]->guiobject_setTargetA(0);
    child[curchild]->guiobject_setTargetSpeed(speed);
    child[curchild]->guiobject_gotoTarget();
  }
  int nextchild = curchild == 1 ? 0 : 1;
  if (child[nextchild]) {
    RootWnd *w = child[nextchild]->guiobject_getRootWnd();
    WASABI_API_SKIN->group_destroy(w);
    child[nextchild] = NULL;
  }
  if (grp && *grp) {
    RootWnd *w = WASABI_API_SKIN->group_create(grp);
    if (w) {
      child[nextchild] = w->getGuiObject();
      w->setParent(this);
      w->setStartHidden(1);
      RECT r;
      getClientRect(&r);
      w->setAlpha(0);
      w->init(this);
      w->setVisible(1);
      w->resize(r.left, r.top, r.right-r.left, r.bottom-r.top);
      child[nextchild]->guiobject_setTargetA(255);
      child[nextchild]->guiobject_setTargetSpeed(speed);
      child[nextchild]->guiobject_gotoTarget();
      id[nextchild] = grp;
    }
  }
  curchild = nextchild;
}
