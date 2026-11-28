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
#include "xuiaddonslist.h"
#include "../bfc/msgbox.h"
#include "addonsmgr.h"
#include "../studio/api.h"

#include "../bfc/svc_enum.h"

// -----------------------------------------------------------------------
char AddOnsListXuiObjectStr[] = "AddOns:List"; // This is the xml tag
char AddOnsListXuiSvcName[] = "AddOns:List XuiObject Service";


// -----------------------------------------------------------------------
AddOnsList::AddOnsList() {
  setPreventMultipleSelection(1);
  addonsmgr.registerList(this);
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "modstype", SKINMODSLIST_SETMODSTYPE, XUI_ATTRIBUTE_IMPLIED); 
  modstype = 0; // global by default
}

// -----------------------------------------------------------------------
AddOnsList::~AddOnsList() {
  addonsmgr.unregisterList(this);
}

// -----------------------------------------------------------------------
int AddOnsList::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SKINMODSLIST_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    // dispatch your xml params here
    case SKINMODSLIST_SETMODSTYPE:
      setModsType(value);
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void AddOnsList::setModsType(const char *t) {
  if (STRCASEEQL(t, "global"))
    modstype = TYPE_GLOBAL;
  else if (STRCASEEQL(t, "perskin"))
    modstype = TYPE_THISSKIN;
  else
    modstype = ATOI(t);
  if (isInited()) addonsmgr.refreshList(this, 0);
}

// -----------------------------------------------------------------------
int AddOnsList::onInit() {
  SKINMODSLIST_PARENT::onInit();
  addColumn("Name",100);
  ListColumn *c = getColumn(0);
  if (c != NULL) c->setDynamic(1);

  addonsmgr.refreshList(this);
  return 1;
}

// -----------------------------------------------------------------------
void AddOnsList::onDoubleClick(int itemnum) {
  addons_about();
}

// -----------------------------------------------------------------------
int AddOnsList::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source) {
  if (STRCASEEQL(action, "addons_add")) { addons_add(x, y); return 1; }
  if (STRCASEEQL(action, "addons_remove")) { addons_remove(); return 1; }
  if (STRCASEEQL(action, "addons_about")) { addons_about(); return 1; }
  return SKINMODSLIST_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, source);
}

// -----------------------------------------------------------------------
int AddOnsList::getSelectedMod() {
  int i = getItemFocused();
  if (i < 0) {
    api->messageBox("No mod selected", "Oops", MSGBOX_OK, NULL, NULL);
    return -1;
  }
  LPARAM p = getItemData(i);
  return p;
}

// -----------------------------------------------------------------------
void AddOnsList::addons_about() {
  int skinpart = getSelectedMod();
  if (skinpart == -1) return;
  addonsmgr.aboutEntry(skinpart);
}

// -----------------------------------------------------------------------
void AddOnsList::addons_add(int x, int y) {
  addonsmgr.add(getModsType(), x, y);
}

// -----------------------------------------------------------------------
void AddOnsList::addons_remove() {
  int skinpart = getSelectedMod();
  if (skinpart == -1) return;
  addonsmgr.removeMod(skinpart);
}

