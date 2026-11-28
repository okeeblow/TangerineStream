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
#include "wndcreate.h"
#include "config.h"
#include "cfgswitch.h"
#include "../common/bucketitem.h"
#include "resource.h"

int ConfigWndCreateSvc::testGuid(GUID g) {
  if (!MEMCMP(&g, &configwac->getGUID(), sizeof(GUID))) return 1;
  return 0;
}

RootWnd *ConfigWndCreateSvc::createWindowByGuid(GUID g, RootWnd *parent) {
  if (!MEMCMP(&g, &configwac->getGUID(), sizeof(GUID))) {
    CfgSwitcher *cfgswitch = new CfgSwitcher();
    cfgswitch->setParent(parent);
    cfgswitch->init(parent);
    cfgswitch->startup();
    configwac->addWnd(cfgswitch);
    return cfgswitch;
  }
  return NULL; 
}

RootWnd *ConfigWndCreateSvc::createWindowOfType(FOURCC windowtype, RootWnd *parent, int n) {
  switch (windowtype) {
    case WindowTypes::BUCKETITEM: {  
      if (n > 0) return NULL;
        BucketItem *bucketitem = new BucketItem;
        bucketitem->setAutoOpen(configwac->getGUID());
        bucketitem->setAutoText(configwac->getName());
        bucketitem->setBitmaps(configwac->gethInstance(), IDB_TAB_NORMAL, NULL, IDB_TAB_HILITED, IDB_TAB_SELECTED);
        bucketitem_list.addItem(bucketitem);
      return bucketitem;
    }
  }
  return NULL;
}

int ConfigWndCreateSvc::destroyWindow(RootWnd *w) {
  if (configwac->haveWnd(w)) {
    configwac->removeWnd(w);
    delete static_cast<CfgSwitcher*>(w);
    return 1;
  }
  if (bucketitem_list.haveItem(w)) { bucketitem_list.removeItem(w); delete static_cast<BucketItem *>(w); }
  return 1;
}
