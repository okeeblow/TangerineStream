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

#include "cfgswitch.h"
#include "cfgtree.h"
#include "cfgwnd.h"

#include <bfc/wnds/buttwnd.h>

#include <bfc/attribs/cfgitem.h>

#include <studio/api.h>
#include <studio/wac.h>

CfgSwitcher::CfgSwitcher() {
  setName("Configurification");
  cfgwnd = new CfgWnd();
  itemlist = new CfgTreeWnd(cfgwnd);
  setChildren(itemlist, cfgwnd);
  setDividerType(DIVIDER_VERTICAL);
  setResizeable(TRUE);
  setDividerPos(SDP_FROMLEFT, 128);
  backup_item = NULL;
}

CfgSwitcher::~CfgSwitcher() {
  // FrameWnd code will delete the child windows
  itemlist->deleteAllItems();
  api->syscb_deregisterCallback(static_cast<SkinCallbackI*>(this));
}

/*void CfgSwitcher::setIconBitmaps(ButtonWnd *button) {
  extern WAComponentClient *the;
  button->setBitmaps(the->gethInstance(), IDB_TAB_NORMAL, NULL, IDB_TAB_HILITED, IDB_TAB_SELECTED);
}*/

int CfgSwitcher::onInit() {
  CFGSWITCHER_PARENT::onInit();

  api->syscb_registerCallback(static_cast<SkinCallbackI*>(this));

  for (int i = 0; ; i++) {
    CfgItem *cfgitem = WASABI_API_CONFIG->config_enumCfgItem(i);
    if (cfgitem == NULL) break;
    insertItem(cfgitem);
  }

  return 1;
}

int CfgSwitcher::skincb_onUnloading() {
//  backup_item = itemlist->getCurItem();
//  cfgwnd->setCfgItem(NULL);
//  itemlist->selectItem(NULL);
  return 1;
}

int CfgSwitcher::skincb_onGuiLoaded() {
//  if (!backup_item) return 1;
//  itemlist->selectItem(backup_item);
//  backup_item = NULL;
  return 1;
}

void CfgSwitcher::insertItem(CfgItem *cfgitem) {
  if (cfgitem == NULL) return;
  // don't add blanks
//CUT  if (cfgitem->getNumAttributes() <= 0 && cfgitem->getConfigXML() == NULL && cfgitem->getNumChildren() <= 0) return;
  // don't add children, just toplevel stuff
  if (cfgitem->getParentGuid() != INVALID_GUID) return;
  TreeItem *ti = itemlist->addTreeItem(new CfgTreeItem(cfgitem), NULL, TRUE, cfgitem->getNumChildren() != 0);
  itemlist->expandItem(ti);
}
