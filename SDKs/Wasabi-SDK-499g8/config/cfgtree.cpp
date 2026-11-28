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

#include "cfgtree.h"
#include "cfgwnd.h"

#include <bfc/attribs/cfgitem.h>

#include <studio/api.h>

CfgTreeItem::CfgTreeItem(CfgItem *item)
  : TreeItemParam<CfgItem *>(item) {
  const char *n = item->getName();
  StringPrintf myguid(item->getGuid());
  setLabel(n ? n : myguid.getValue());
  if (n) myguid.prepend(StringPrintf("%s: ", n));
  setTip(myguid);
}

void CfgTreeItem::onTreeAdd() {
  CfgItem *cfgitem = getParam();
  for (int i = 0; ; i++) {
    CfgItem *child = WASABI_API_CONFIG->config_enumCfgItem(i);
    if (child == NULL) break;
    if (child->getParentGuid() != cfgitem->getGuid()) continue;

#if 1
	if (child->getNumAttributes() > 0 ||
        child->getNumChildren() > 0 ||
        child->getConfigXML() != NULL)
#endif
	{
      getTree()->addTreeItem(new CfgTreeItem(child), this, TRUE, child->getNumChildren() != 0);
    }
  }
}

CfgTreeWnd::CfgTreeWnd(CfgWnd *_cfgwnd) {
  cfgwnd = _cfgwnd;
  setSorted(FALSE);
};

CfgTreeWnd::~CfgTreeWnd() {
  api->syscb_deregisterCallback(static_cast<MetaCallbackI*>(this));
  if (last_sel.isempty())
    WASABI_API_CONFIG->setStringPrivate("last", NULL);
  else
    WASABI_API_CONFIG->setStringPrivate("last", last_sel);
}

int CfgTreeWnd::onInit() {
  CFGTREEWND_PARENT::onInit();
  api->syscb_registerCallback(static_cast<MetaCallbackI*>(this));
  return 1;
}

void CfgTreeWnd::onItemSelected(TreeItem *item) {
  CFGTREEWND_PARENT::onItemSelected(item);

  CfgItem *cfgitem = (static_cast<CfgTreeItem *>(item))->getParam();
  cfgwnd->setCfgItem(cfgitem);

  last_sel = item->getLabel();
}

void CfgTreeWnd::metacb_onPostScan() {
  api->syscb_deregisterCallback(static_cast<MetaCallbackI*>(this));
  char last[4096];
  WASABI_API_CONFIG->getStringPrivate("last", last, 4096, "");
  if (*last) {
    TreeItem *ti = getByLabel(NULL, last);
    if (ti != NULL) selectItemDeferred(ti);
  }
}
