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
#include "wndholder.h"
#include "../../studio/services/svc_wndcreate.h"
#include "../../studio/api.h"
#include "../svc_enum.h"
#include "../../common/wndcb.h"


#pragma SELF("lone", "needs to dispatch Layout & Containers!")
#include "../../studio/container.h"
#include "../../studio/layout.h"

#define CBCLASS WindowHolderI
START_DISPATCH;
  CB(WNDHOLDER_ONINSERTWINDOW, onInsertWindow);
  VCB(WNDHOLDER_ONREMOVEWINDOW, onRemoveWindow);
  CB(WNDHOLDER_WANTGUID, wantGuid);
  CB(WNDHOLDER_WANTGROUP, wantGroup);
  CB(WNDHOLDER_GETROOTWNDPTR, getRootWndPtr);
  CB(WNDHOLDER_GETCURGROUPID, getCurGroupId);
  CB(WNDHOLDER_GETCURGUID, getCurGuid);
  CB(WNDHOLDER_GETCURROOTWND, getCurRootWnd);
  CB(WNDHOLDER_GETCURID, getCurId);
  CB(WNDHOLDER_ISGENERICGUID, acceptsGenericGuid);
  CB(WNDHOLDER_ISGENERICGROUP, acceptsGenericGroup);
  VCB(WNDHOLDER_CANCELDEFERREDREMOVE, cancelDeferredRemove);
  VCB(WNDHOLDER_ONNEEDRELOADGRP, wndholder_onNeedReloadGroup);
  CB(WNDHOLDER_WANTAUTOFOCUS, wndholder_wantAutoFocus);
  CB(WNDHOLDER_ISAUTOAVAILABLE, wndholder_isAutoAvailable);
END_DISPATCH;

WindowHolderI::WindowHolderI() {
  wnd = NULL; 
  dr = NULL;
  generic_guid = 1;
  generic_group = 1;
  cur_guid = INVALID_GUID;
  cur_groupid = NULL;
  wc_svc = NULL;
  WASABI_API_WNDMGR->wndholder_register(this);
}

WindowHolderI::~WindowHolderI() {
  delete dr;
  if (wnd != NULL) {
    if (wc_svc) {
      RootWnd *w = wnd;
      wnd = NULL;
      wc_svc->destroyWindow(w);
      SvcEnum::release(wc_svc);
      wc_svc = NULL;
    } else {
      RootWnd *w = wnd;
      wnd = NULL;
      WASABI_API_SKIN->group_destroy(w);
    }
  }
  accepted_groups.deleteAll();
  accepted_guids.deleteAll();
  WASABI_API_WNDMGR->wndholder_unregister(this);
}

RootWnd *WindowHolderI::onInsertWindow(GUID g, const char *groupid) {
  cancelDeferredRemove();

  defered_guid = INVALID_GUID;
  if (wnd != NULL) return NULL;

  cur_groupid = groupid;
  if (g != INVALID_GUID) {
    cur_guid = g;
    char cguid[256];
    nsGUID::toChar(g, cguid);
    cur_id = cguid;
  } else
    cur_id = groupid;

  wnd = createWindow(g == INVALID_GUID ? NULL : &g, groupid);
  if (!wnd) {
    cur_guid = INVALID_GUID;
    cur_id = "";
    cur_groupid = "";
  }
  if (wnd) {
    onInsert(wnd, cur_id);
  } else {
    if (g != INVALID_GUID) {
      defered_guid = g;
    }
  }
  return wnd;
}

void WindowHolderI::onRemoveWindow(int deferred) {
  if (deferred) {
    if (!dr)
      dr = new DeferredRemove(this);
    dr->post();
    return;
  }
  if (wnd != NULL) {
    RootWnd *w = getCurRootWnd();
    onRemove(w, cur_id);
    destroyWindow();
    cur_guid = INVALID_GUID;
    cur_groupid = NULL;
    defered_guid = INVALID_GUID;
  }
}

void WindowHolderI::cancelDeferredRemove() {
  delete dr;
  dr = NULL;
}

void WindowHolderI::wndholder_onNeedReloadGroup(const char *id) {
  if (cur_groupid.isempty()) return;
  RootWnd *w = getCurRootWnd();
  if (w == NULL) return;
  if (w->isInited() && STRCASEEQL(cur_groupid, id)) {
    onRemove(w, cur_id);
    destroyWindow();
    createWindow(&INVALID_GUID, id);
    onInsert(wnd, cur_id);
  }
}

RootWnd *WindowHolderI::createWindow(const GUID *g, const char *groupid) {
  ASSERT(wnd == NULL);
  if (g != NULL && *g != INVALID_GUID) {
    wc_svc = WindowCreateByGuidEnum(*g).getFirst();
    if (wc_svc)
      wnd = wc_svc->createWindowByGuid(*g, getRootWndPtr());
  } else if (groupid != NULL) {
    wc_svc = NULL;
    wnd = WASABI_API_SKIN->group_create(groupid);
  }
  if (wnd) {
    if (!wnd->isInited()) {
      if (!wnd->getParent()) wnd->setParent(getRootWndPtr());
      wnd->init(getRootWndPtr());
    }
  }
  return wnd;
}

void WindowHolderI::destroyWindow() {
  ASSERT(wnd != NULL);

  RootWnd *w = wnd->getDesktopParent();

  if (wc_svc) {
    RootWnd *w = wnd;
    wnd = NULL;
    wc_svc->destroyWindow(w);
    SvcEnum::release(wc_svc);
    wc_svc = NULL;
  } else {
    RootWnd *w = wnd;
    wnd = NULL;
    WASABI_API_SKIN->group_destroy(w);
  }

  if (w != NULL) {
    if (w->getInterface(layoutGuid)) {
      static_cast<Layout *>(w)->updateTransparency();
    }
  }

}

void WindowHolderI::addAcceptGuid(GUID g) {
  accepted_guids.addItem(new GUID(g));
}

void WindowHolderI::addAcceptGroup(const char *groupid) {
  accepted_groups.addItem(new String(groupid));
}

void WindowHolderI::setAcceptAllGuids(int tf) {
  generic_guid = tf;
}

void WindowHolderI::setAcceptAllGroups(int tf) {
  generic_group = tf;
}

int WindowHolderI::wantGuid(GUID g) {
  if (acceptsGenericGuid()) return 1;
  for (int i=0;i<accepted_guids.getNumItems();i++) {
    if (*accepted_guids.enumItem(i) == g) return 1;
  }
  return 0;
}

int WindowHolderI::wantGroup(const char *groupid) {
  if (acceptsGenericGroup()) return 1;
  for (int i=0;i<accepted_groups.getNumItems();i++) {
    if (STRCASEEQL(accepted_groups.enumItem(i)->getValue(), groupid)) return 1;
  }
  return 0;
}

GUID *WindowHolderI::getFirstAcceptedGuid() {
  if (accepted_guids.getNumItems() == 0) return NULL;
  return accepted_guids.enumItem(0);
}

const char *WindowHolderI::getFirstAcceptedGroup() {
  if (accepted_guids.getNumItems() == 0) return NULL;
  return accepted_groups.enumItem(0)->getValue();
}

int WindowHolderI::wndholder_wantAutoFocus() {
  return 1;
}

WindowHolderWnd::WindowHolderWnd() {
  autoavail = 1;
  autoopen = 1;
  autoclose = 0;
  nocmdbar = 0;
  noanim = 0;
  has_wnd = 0;
  autofocus = 1;
  WASABI_API_SYSCB->syscb_registerCallback(this);
}

WindowHolderWnd::~WindowHolderWnd() {
  if (has_wnd) {
    notifyOnRemove();
  }
  WASABI_API_SYSCB->syscb_deregisterCallback(this);
}

int WindowHolderWnd::onResize() {
  WINDOWHOLDER_PARENT::onResize();
  if (getCurRootWnd()) {
    RECT r;
    getClientRect(&r);
    if (!getCurRootWnd()->handleRatio())
      multRatio(&r);
    getCurRootWnd()->resize(r.left, r.top, r.right-r.left, r.bottom-r.top);
  }
  return 1;
}

int WindowHolderWnd::handleRatio() {
  return 1;
}

int WindowHolderWnd::handleDesktopAlpha() {
  if (getCurRootWnd()) return getCurRootWnd()->handleDesktopAlpha();
  return 1;
}

int WindowHolderWnd::handleTransparency() {
  if (getCurRootWnd()) return getCurRootWnd()->handleTransparency();
  return 1;
}

int WindowHolderWnd::onInit() {
  WINDOWHOLDER_PARENT::onInit();
  if (isVisible() && autoopen && getFirstAcceptedGuid()) {
    onInsertWindow(*getFirstAcceptedGuid(), NULL);
  } else if (isVisible() && autoopen && getFirstAcceptedGroup()) {
    onInsertWindow(INVALID_GUID, getFirstAcceptedGroup());
  }
  return 1;
}

#define DC_NOTIFYONREMOVE 0x205

void WindowHolderWnd::onRemove(RootWnd *w, const char *id) {
  RootWnd *dw = getDesktopParent();
  if (dw) dw->removeMinMaxEnforcer(this);
  postDeferredCallback(DC_NOTIFYONREMOVE);
}

int WindowHolderWnd::onDeferredCallback(int p1, int p2) {
  if (p1 == DC_NOTIFYONREMOVE) {
    notifyOnRemove();
  } else return WINDOWHOLDER_PARENT::onDeferredCallback(p1, p2);
  return 1;
}

void WindowHolderWnd::onInsert(RootWnd *w, const char *id) {
  if (isPostOnInit())
    onResize();
  RootWnd *dw = getDesktopParent();
  if (dw) dw->addMinMaxEnforcer(this);
  notifyOnInsert();
  if (wndholder_wantAutoFocus()) w->setFocus();
}

int WindowHolderWnd::getPreferences(int what) {
  if (getCurRootWnd()) return getCurRootWnd()->getPreferences(what);
  return WINDOWHOLDER_PARENT::getPreferences(what);
}

void WindowHolderWnd::notifyOnRemove() {
  has_wnd = 0;
   Layout *l = getGuiObject()->guiobject_getParentLayout();
  if (l != NULL) {
    Container *c = l->getParentContainer();
    if (c != NULL) {
      c->notifyRemoveContent(this);
    }
  }
}

void WindowHolderWnd::notifyOnInsert() {
  has_wnd = 1;
   Layout *l = getGuiObject()->guiobject_getParentLayout();
  if (l != NULL) {
    Container *c = l->getParentContainer();
    if (c != NULL) {
      c->notifyAddContent(this, getCurGroupId(), getCurGuid());
    }
  }
}

int WindowHolderWnd::onGroupChange(const char *grpid) {
  WINDOWHOLDER_PARENT::onGroupChange(grpid);
  wndholder_onNeedReloadGroup(grpid);
  return 1;
}

void WindowHolderWnd::onSetVisible(int show) {
  if (show && getCurRootWnd() == NULL) {
    if (autoopen && getFirstAcceptedGuid()) {
      onInsertWindow(*getFirstAcceptedGuid(), NULL);
    } else if (autoopen && getFirstAcceptedGroup()) {
      onInsertWindow(INVALID_GUID, getFirstAcceptedGroup());
    }
  } else if (!show && getCurRootWnd() != NULL) {
    if (autoclose && WASABI_API_SKIN->skin_getVersion() >= 1.0)
      onRemoveWindow(0);
  }
  if (getDeferedGuid() != INVALID_GUID) {
    if (show) {
      #ifdef ON_CREATE_EXTERNAL_WINDOW_GUID
        int y;
        ON_CREATE_EXTERNAL_WINDOW_GUID(getDeferedGuid(), y);
      #endif
    } 
  }
  WINDOWHOLDER_PARENT::onSetVisible(show);
}

int WindowHolderWnd::wndholder_wantAutoFocus() {
  return autofocus;
}

