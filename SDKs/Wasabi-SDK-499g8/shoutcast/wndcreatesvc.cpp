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
#include "wndcreatesvc.h"
#include "../common/bucketitem.h"
#include "shoutcastwac.h"
#include "resource.h"
#include "shoutcastwnd.h"

static RootWnd *shoutcast;

int ShoutcastWndCreateSvc::testGuid(GUID g) {
  if (!MEMCMP(&g, &shoutcastwac->getGUID(), sizeof(GUID))) return 1;
  return 0;
}

RootWnd *ShoutcastWndCreateSvc::createWindowByGuid(GUID g, RootWnd *parent) {
  if (!MEMCMP(&g, &shoutcastwac->getGUID(), sizeof(GUID))) {
    shoutcast = shoutcastwac->createWindow(0, parent);
    return shoutcast;
  }
  return NULL; 
}

RootWnd *ShoutcastWndCreateSvc::createWindowOfType(const char *windowtype, RootWnd *parent, int n) {
  if (STRCASEEQLSAFE(windowtype, BUCKETITEM)) {
    if (n > 0) return NULL;
    bucketitem = new BucketItem;
    bucketitem->setAutoOpen(shoutcastwac->getGUID());
    bucketitem->setAutoText(shoutcastwac->getName());
    bucketitem->setBitmaps(shoutcastwac->gethInstance(), IDB_TAB_NORMAL, NULL, IDB_TAB_HILITED, IDB_TAB_SELECTED);
    return bucketitem;
  }
  return NULL;
}

int ShoutcastWndCreateSvc::destroyWindow(RootWnd *w) {
  if (w == shoutcast) { delete static_cast<ShoutcastWnd *>(w); shoutcast = NULL; }
  if (w == bucketitem) { delete static_cast<BucketItem *>(w); bucketitem = NULL; }
  return 1;
}
