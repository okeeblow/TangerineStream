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
#include "wndmgrapi.h"
#include "../../studio/autopopup.h"
#include "../../studio/skinembed.h"
#include "../../studio/animate.h"
#include "../../studio/wndtrack.h"
#include "../../studio/skinparse.h"
#include "../../bfc/msgbox.h"
#include "../../studio/varmgr.h"

wndmgr_api *wndManagerApi = NULL;

WndMgrApi::WndMgrApi() {
  skinEmbedder = new SkinEmbedder();
  windowTracker = new WindowTracker();
}

WndMgrApi::~WndMgrApi() {
  delete skinEmbedder;
  skinEmbedder = NULL;
  delete windowTracker;
  windowTracker = NULL;
}

void WndMgrApi::wndTrackAdd(RootWnd *wnd) {
  if (wnd == NULL) {
    DebugString("wndTrackAdd, illegal param : wnd == NULL\n");
    return;
  }
  windowTracker->addWindow(wnd);
}

void WndMgrApi::wndTrackRemove(RootWnd *wnd) {
  if (wnd == NULL) {
    DebugString("wndTrackAdd, illegal param : wnd == NULL\n");
    return;
  }
  windowTracker->removeWindow(wnd);
}

BOOL WndMgrApi::wndTrackDock(RootWnd *wnd, RECT *r, int mask) {
  return windowTracker->autoDock(wnd, r, mask);
}

BOOL WndMgrApi::wndTrackDock2(RootWnd *wnd, RECT *r, RECT *orig_r, int mask) {
  return windowTracker->autoDock(wnd, r, orig_r, mask);
}

void WndMgrApi::wndTrackStartCooperative(RootWnd *wnd) {
  windowTracker->startCooperativeMove(wnd);
}

void WndMgrApi::wndTrackEndCooperative(void) {
  windowTracker->endCooperativeMove();
}

int WndMgrApi::wndTrackWasCooperative(void) {
  return windowTracker->wasCooperativeMove();
}

void WndMgrApi::wndTrackInvalidateAll() {
  windowTracker->invalidateAllWindows();
}

int WndMgrApi::skinwnd_toggleByGuid(GUID g, const char *prefered_container, int container_flag, RECT *sourceanimrect, int transcient) {
  return skinEmbedder->toggle(g, prefered_container, container_flag, sourceanimrect, transcient);
}

int WndMgrApi::skinwnd_toggleByGroupId(const char *groupid, const char *prefered_container, int container_flag, RECT *sourceanimrect, int transcient) {
  return skinEmbedder->toggle(groupid, prefered_container, container_flag, sourceanimrect, transcient);
}

RootWnd *WndMgrApi::skinwnd_createByGuid(GUID g, const char *prefered_container, int container_flag, RECT *sourceanimrect, int transcient, int starthidden, int *isnew) {
  return skinEmbedder->create(g, prefered_container, container_flag, sourceanimrect, transcient, starthidden, isnew);
}

RootWnd *WndMgrApi::skinwnd_createByGroupId(const char *groupid, const char *prefered_container, int container_flag, RECT *sourceanimrect, int transcient, int starthidden, int *isnew) {
  return skinEmbedder->create(groupid, prefered_container, container_flag, sourceanimrect, transcient, starthidden, isnew);
}

void WndMgrApi::skinwnd_destroy(RootWnd *w, RECT *destanimrect) {
  skinEmbedder->destroy(w, destanimrect);
}

int WndMgrApi::skinwnd_getNumByGuid(GUID g) {
  return skinEmbedder->getNumItems(g);
}

RootWnd *WndMgrApi::skinwnd_enumByGuid(GUID g, int n) {
  return skinEmbedder->enumItem(g, n);
}

int WndMgrApi::skinwnd_getNumByGroupId(const char *groupid) {
  return skinEmbedder->getNumItems(groupid);
}

RootWnd *WndMgrApi::skinwnd_enumByGroupId(const char *groupid, int n) {
  return skinEmbedder->enumItem(groupid, n);
}

void WndMgrApi::skinwnd_attachToSkin(RootWnd *w, int side, int size) {
  skinEmbedder->attachToSkin(w, side, size);
}

ScriptObject *WndMgrApi::skin_getContainer(const char *container_name) {
  Container *c = SkinParser::getContainer(container_name);
  if (c != NULL) return c->getScriptObject();
  return NULL;
}

ScriptObject *WndMgrApi::skin_getLayout(ScriptObject *container, const char *layout_name) {
  ASSERT(container != NULL);
  Container *c = static_cast<Container *>(container->vcpu_getInterface(containerGuid));
  if (c != NULL) {
    Layout *l = c->getLayout(layout_name);
    if (l != NULL) return l->getScriptObject();
  }
  return NULL;
}

void WndMgrApi::wndholder_register(WindowHolder *wh) {
  skinEmbedder->registerWindowHolder(wh);
}

void WndMgrApi::wndholder_unregister(WindowHolder *wh) {
  skinEmbedder->unregisterWindowHolder(wh);
}

int WndMgrApi::messageBox(const char *txt, const char *title, int flags, const char *not_anymore_identifier, RootWnd *parentwnd) {
#ifdef WASABI_WNDMGR_OSMSGBOX
  #ifdef WIN32
    
    int f = 0;
    if (flags & MSGBOX_OK) f |= MB_OK;
    if (flags & MSGBOX_CANCEL) f |= MB_OKCANCEL;
    if (flags & MSGBOX_YES) f |= MB_YESNO;
    if ((flags & MSGBOX_YES) && (flags & MSGBOX_CANCEL)) f |= MB_YESNOCANCEL;

    int r = MessageBox(parentwnd ? parentwnd->gethWnd() : NULL, txt, title, f);

    if (r == IDOK) r = MSGBOX_OK;
    if (r == IDCANCEL) r = MSGBOX_CANCEL;
    if (r == IDYES) r = MSGBOX_YES;
    if (r == IDNO) r = MSGBOX_NO;

    return r;

  #else
  // port me!
  #endif
#else
  MsgBox _mb(txt, title, flags, not_anymore_identifier);
  return _mb.run();
#endif
}  

void WndMgrApi::drawAnimatedRects(const RECT *r1, const RECT *r2) {
  if (r1 == NULL) {
    DebugString("drawAnimatedRects, illegal param : r1 == NULL\n");
    return;
  }
  if (r2 == NULL) {
    DebugString("drawAnimatedRects, illegal param : r2 == NULL\n");
    return;
  }
  AnimatedRects::draw(r1, r2);
}

int WndMgrApi::autopopup_registerGuid(GUID g, const char *desc, const char *prefered_container, int container_flag) {
  return AutoPopup::registerGuid(SKINPARTID_NONE, g, desc, prefered_container, container_flag);
}

int WndMgrApi::autopopup_registerGroupId(const char *groupid, const char *desc, const char *prefered_container, int container_flag) {
  return AutoPopup::registerGroupId(SKINPARTID_NONE, groupid, desc, prefered_container, container_flag);
}

void WndMgrApi::autopopup_unregister(int id) {
  AutoPopup::unregister(id);
}

int WndMgrApi::autopopup_getNumGuids() {
  return AutoPopup::getNumGuids();
}

GUID WndMgrApi::autopopup_enumGuid(int n) {
  return AutoPopup::enumGuid(n);
}

int WndMgrApi::autopopup_getNumGroups() {
  return AutoPopup::getNumGroups();
}

const char *WndMgrApi::autopopup_enumGroup(int n) {
  return AutoPopup::enumGroup(n);
}

const char *WndMgrApi::autopopup_enumGuidDescription(int n) {
  return AutoPopup::enumGuidDescription(n);
}

const char *WndMgrApi::autopopup_enumGroupDescription(int n) {
  return AutoPopup::enumGroupDescription(n);
}

const char *WndMgrApi::varmgr_translate(const char *str) {
  String *s = PublicVarManager::translate_nocontext(str);
  ret = s;
  delete s;
  return ret.getValueSafe();
}

Stack<RootWnd*> WndMgrApi::modal_wnd_stack;

RootWnd *WndMgrApi::getModalWnd() {
  if (!modal_wnd_stack.peek()) return NULL;
  return modal_wnd_stack.top();
}

void WndMgrApi::pushModalWnd(RootWnd *wnd) {
  modal_wnd_stack.push(wnd);
}

void WndMgrApi::popModalWnd(RootWnd *wnd) {
  if (getModalWnd() != wnd) return;
  modal_wnd_stack.pop();
}

Container *WndMgrApi::newDynamicContainer(const char *name, int transcient) {
  return SkinParser::newDynamicContainer(name, transcient);
}
