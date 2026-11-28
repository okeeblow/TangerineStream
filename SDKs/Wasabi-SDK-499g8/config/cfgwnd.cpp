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

#include "cfgwnd.h"
#include "attredit.h"

#include <bfc/wnds/form/cells.h>
#include <studio/api.h>
#include <bfc/notifmsg.h>

CfgWnd::CfgWnd() {
  cfgitem = NULL;
}

CfgWnd::~CfgWnd() { }

int CfgWnd::onInit() {
  CFGWND_PARENT::onInit();
  populateAttribs();
  return 1;
}

int CfgWnd::onResize() {
  CFGWND_PARENT::onResize();
  invalidate();
  return 1;
}

CfgItem *CfgWnd::getCfgItem() {
   return cfgitem;
}

void CfgWnd::setCfgItem(CfgItem *item) {
  ASSERTPR(isInited(), "must init before setting cfgitem");

  viewer_delAllItems();

  if (item != NULL) {
    Dependent *dep = item->getDependencyPtr();
    if (dep != NULL)
      viewer_addViewItem(dep);
  }
  cfgitem = item;

  if (cfgitem != NULL) {
    setName(cfgitem->getName());
    showLabel(TRUE);
  } else {
    showLabel(FALSE);
  }

  populateAttribs();
}

static struct { int type; const char *name; } grouplist[] = {
  { AttributeType::INT, "studio.configgroup.int" },
  { AttributeType::BOOL, "studio.configgroup.bool" },
  { AttributeType::STRING, "studio.configgroup.string" },
  { AttributeType::FLOAT, "studio.configgroup.float" },
  { AttributeType::NONE },	// this has to be last
};

void CfgWnd::populateAttribs() {
  if (!isInited()) return;

  deleteAll();

  if (cfgitem == NULL) {
    newCell(new CenteredTextCell("[nothing selected]"));
    recalcForm();
    return;
  }

  String cfgxml = cfgitem->getConfigXML();
  if (!cfgxml.isempty()) {
//    RootWnd *group = api->group_create(cfgxml);
    RootWnd *group = api->group_create_cfg(cfgxml, cfgitem, "");
    if (group != NULL) {
      CfgGroupCell *cg = new CfgGroupCell(cfgitem, group);
      cg->setRootWndContent(group);
      newCell(cg);
    }
  } else {
    int n = cfgitem->getNumAttributes();
    if (n == 0) {
      newCell(new CenteredTextCell("[no attributes present]"));
    } else for (int i = 0; i < n; i++) {
      String attrname = cfgitem->enumAttribute(i);
/*    TextCell *label = new TextCell(attrname);
    newCell(label);
    label->setWidthProp(45);
    label->setHeightFixed(16);*/

/*    AttrEdit *ae = AttrEdit::create(cfgitem, attrname);
    newCell(new AttrEditCell(ae));*/

      RootWnd *group = NULL;
      int attrtype = cfgitem->getAttributeType(attrname);

      // try to make theirs first
      const char *groupname = cfgitem->getAttributeConfigGroup(attrname);
      if (groupname != NULL)
        group = api->group_create_cfg(groupname, cfgitem, attrname);

      // try our list of default names if they didn't supply one
      if (group == NULL) {
        for (int i = 0; grouplist[i].type != AttributeType::NONE; i++) {
          if (grouplist[i].type == attrtype) {
            group = api->group_create_cfg(grouplist[i].name, cfgitem, attrname);
            break;
          }
        }
      }

      if (group) {
        CfgGroupCell *cg = new CfgGroupCell(cfgitem, group, attrname);
        cg->setWidthProp(100);
        cg->setHeightFixed(group->getPreferences(SUGGESTED_H));
        cg->setRootWndContent(group);
        newCell(cg);
      } else {
        FormCell *cell = new TextCell(StringPrintf("%s [unknown type]", attrname.v()));
        cell->setHeightFixed(16);
        newCell(cell);
      }

      nextRow();
    }
  }

  recalcForm();
}
