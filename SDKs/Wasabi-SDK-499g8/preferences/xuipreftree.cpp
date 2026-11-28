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
#include "xuipreftree.h"
#include "../bfc/wnds/customobject.h"
#include "../studio/api.h"
#include "preferences.h"

#define CB_OPENSYSTEM 0x5423

char PreferencesTreeXuiObjectStr[] = "Preferences:Tree"; 
char PreferencesTreeXuiSvcName[] = "Preferences:Tree XuiObject Service"; 

// -----------------------------------------------------------------------
PreferencesTree::PreferencesTree() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "target", PREFERENCESTREE_SETTARGET, XUI_ATTRIBUTE_IMPLIED);
  getScriptObject()->vcpu_setInterface(prefTreeGuid, static_cast<PreferencesTree*>(this));
  group = NULL;
  setAutoCollapse(0);
  target = "wasabi.preferences.configpane";
}

// -----------------------------------------------------------------------
PreferencesTree::~PreferencesTree() {
  if (group != NULL) 
    api->group_destroy(group);
}

// -----------------------------------------------------------------------
int PreferencesTree::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return PREFERENCESTREE_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case  PREFERENCESTREE_SETTARGET:
      target = value;
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

int PreferencesTree::onInit() {
  fillTree();
  return PREFERENCESTREE_PARENT::onInit();
}

// -----------------------------------------------------------------------
void PreferencesTree::fillTree() {

  setSorted(FALSE);

  int i;
  for (i=0;i<api->preferences_getNumGroups();i++) {
    GUID thisguid;
    GUID parentguid;
    const char *desc;
    const char *group = api->preferences_enumGroup(i, &desc, &thisguid, &parentguid);
    screens.addItem(new PreferenceScreen(group, desc, thisguid, parentguid));
  }

  foreach(screens)
    PreferenceScreen *s = screens.getfor();
    if (s->getParentGuid() == INVALID_GUID)
      addTreeItem(s, NULL, FALSE, 0);
  endfor;

  foreach(screens)
    PreferenceScreen *s = screens.getfor();
    if (s->getParentGuid() != INVALID_GUID) {
      GUID target = s->getParentGuid();
      foreach(screens)
        PreferenceScreen *p = screens.getfor();
        if (p->getGuid() == target) {
          addTreeItem(s, p, FALSE, 0);
          p->setChildTab(TAB_AUTO);
        }
      endfor;
    }
  endfor;

  for (i=0;i<getNumRootItems();i++)
    enumRootItem(i)->expand();

  if (!disallow_select_default)
    postDeferredCallback(CB_OPENSYSTEM, 0);
}

// -----------------------------------------------------------------------
int PreferencesTree::onDeferredCallback(int p1, int p2) {
  if (p1 == CB_OPENSYSTEM) {
    openGroup("winamp.preferences.general", 1);
    return 1;
  }
  return PREFERENCESTREE_PARENT::onDeferredCallback(p1, p2);
}

// -----------------------------------------------------------------------
void PreferencesTree::onItemSelected(TreeItem *item) {
  PREFERENCESTREE_PARENT::onItemSelected(item);
  ASSERT(item != NULL);
  const char *groupid = (static_cast<PreferenceScreen *>(item))->getGroupId();

  openGroup(groupid);
  setFocus();
}


// -----------------------------------------------------------------------
void PreferencesTree::openGroup(const char *groupid, int reselect) {
  ASSERT(groupid != NULL);

  if (STRCASEEQLSAFE(groupid, curGroup)) return;

  curGroup = groupid;

  OutputDebugString(StringPrintf("Preferences - opening group %s\n", groupid));

  RootWnd *old = group; group = NULL;
  if (old != NULL) {
    old->setVisible(0);
  }

  GuiObject *go = findObject(target);
  if (go == NULL) return;
  CustomObject *co = static_cast<CustomObject *>(go->guiobject_getScriptObject()->vcpu_getInterface(customObjectGuid));
  if (co != NULL) {
    group = api->group_create(groupid);
    co->customobject_setRootWnd(group);
  }

  //group->getDesktopParent()->cascadeRepaint(0);

  if (old != NULL)
    api->group_destroy(old);

  if (reselect) {
    for (int i=0;i<getNumVisibleItems();i++) {
      TreeItem *it = enumVisibleItems(i);
      const char *id = (static_cast<PreferenceScreen *>(it))->getGroupId();
      if (STRCASEEQLSAFE(groupid, id)) {
        selectItem(it);
        break;
      }
    }
  }
}
