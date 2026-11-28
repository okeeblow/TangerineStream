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
#include "wndapi.h"
#include "../../bfc/rootwnd.h"
#include "../../bfc/canvas.h"
#include "../../studio/main.h"
#include "../../studio/genwnd.h"
#ifdef WASABI_COMPILE_SKIN
#include "../../studio/skin.h"
#endif
#include "../../studio/deactivatemgr.h"
#include "../../studio/wndtrack.h"
#include "../../studio/popexitchecker.h"
#include "../../studio/cbmgr.h"
#include "../../studio/consolecb.h"
#include "../../studio/keyboard.h"
#ifdef WASABI_COMPILE_SKIN
  #include "../../studio/groupmgr.h"
#endif
#ifdef WASABI_COMPILE_PAINTSETS
#ifdef WASABI_COMPILE_IMGLDR
  #include "../../studio/paintset.h"
#endif
#endif

wnd_api *wndApi = NULL;

#ifndef ODS
#define ODS(msg1, msg2) __ODS(__LINE__, msg1, msg2, 0);
#endif

static Stack<RootWnd*> modal_wnd_stack;

static void __ODS(int line, char *message1, char *message2, int severity) {
  String s = StringPrintf("wndApi(%d): %s: %s\n", line, message1, message2);
#ifdef WIN32
  DebugString("%s\n", s);
#endif
  CallbackManager::issueCallback(SysCallback::CONSOLE, ConsoleCallback::DEBUGMESSAGE, severity, reinterpret_cast<int>(s.getValue()));
}

WndApi::WndApi() {
  popupExitChecker = new PopupExitChecker();
}

WndApi::~WndApi() {
  delete popupExitChecker;
  popupExitChecker = NULL;
}

void WndApi::main_setRootWnd(RootWnd *w) {
  genericwnd = w;
}

RootWnd *WndApi::main_getRootWnd() {
  return genericwnd;
}

RootWnd *WndApi::getModalWnd() {
  if (!modal_wnd_stack.peek()) return NULL;
  return modal_wnd_stack.top();
}

void WndApi::pushModalWnd(RootWnd *w) {
  modal_wnd_stack.push(w);
}

void WndApi::popModalWnd(RootWnd *w) {
  if (getModalWnd() != w) return;
  modal_wnd_stack.pop();
}

RootWnd *WndApi::rootWndFromPoint(POINT *pt) {
  return WindowTracker::rootWndFromPoint(pt);
}

void WndApi::registerRootWnd(RootWnd *wnd) {
  WindowTracker::addRootWnd(wnd);
}

void WndApi::unregisterRootWnd(RootWnd *wnd) {
  WindowTracker::removeRootWnd(wnd);
}

int WndApi::rootwndIsValid(RootWnd *wnd) {
  return windowTracker->checkWindow(wnd);
}
void WndApi::hookKeyboard(RootWnd *hooker) {
  Keyboard::hookKeyboard(hooker);
}

void WndApi::unhookKeyboard(RootWnd *hooker) {
  Keyboard::unhookKeyboard(hooker);
}

void WndApi::kbdReset() {
  Keyboard::reset();
}

// so when a key is pressed in a basewnd, it is first intercepted and sent to these functions to check if the system should handle it instead of the wnd...

int WndApi::interceptOnChar(unsigned int c) {
  return Keyboard::interceptOnChar(c);
}

int WndApi::interceptOnKeyDown(int k) {
  return Keyboard::interceptOnKeyDown(k);
}

int WndApi::interceptOnKeyUp(int k) {
  return Keyboard::interceptOnKeyUp(k);
}

int WndApi::interceptOnSysKeyDown(int k, int kd) {
  return Keyboard::interceptOnSysKeyDown(k, kd);
}

int WndApi::interceptOnSysKeyUp(int k, int kd) {
  return Keyboard::interceptOnSysKeyUp(k, kd);
}

// ... if not, then it is sent to the wnd, and if the wnd doesn't need it, it is then forwarded to the system again, for default handling :

int WndApi::forwardOnChar(RootWnd *from, unsigned int c, int kd) {
  return Keyboard::onForwardOnChar(from, c, kd);
}

int WndApi::forwardOnKeyDown(RootWnd *from, int k, int kd) {
  return Keyboard::onForwardOnKeyDown(from, k, kd);
}

int WndApi::forwardOnKeyUp(RootWnd *from, int k, int kd) {
  return Keyboard::onForwardOnKeyUp(from, k, kd);
}

int WndApi::forwardOnSysKeyDown(RootWnd *from, int k, int kd) {
  return Keyboard::onForwardOnSysKeyDown(from, k, kd);
}

int WndApi::forwardOnSysKeyUp(RootWnd *from, int k, int kd) {
  return Keyboard::onForwardOnSysKeyUp(from, k, kd);
}

int WndApi::forwardOnKillFocus() {
  return Keyboard::onForwardOnKillFocus();
}

void WndApi::popupexit_register(PopupExitCallback *cb, RootWnd *watched) {
  if (popupExitChecker == NULL) return;
  popupExitChecker->registerCallback(cb, watched);
}
void WndApi::popupexit_deregister(PopupExitCallback *cb) {
  if (popupExitChecker == NULL) return;
  popupExitChecker->deregisterCallback(cb);
}

int WndApi::popupexit_check(RootWnd *w) {
  if (popupExitChecker == NULL) return 0;
  return popupExitChecker->check(w);
}

void WndApi::popupexit_signal() {
  if (popupExitChecker == NULL) return;
  popupExitChecker->signal();
}

#define RenderBaseTexture renderBaseTexture //CUT
void WndApi::skin_renderBaseTexture(RootWnd *basetexturewnd, CanvasBase *c, const RECT *r, RootWnd *destwnd, int alpha) {
  if (c == NULL) {
    ODS("illegal param", "c == NULL");
    return;
  }
  if (basetexturewnd == NULL) {
    ODS("illegal param", "base == NULL");
    BaseCloneCanvas canvas(c);
    canvas.fillRect(r, 0xFF00FF);
    return;
  }
  if (destwnd == NULL) {
    ODS("illegal param", "destwnd == NULL");
    return;
  }
  renderBaseTexture(basetexturewnd, c, *r, destwnd, alpha);
}

void WndApi::skin_registerBaseTextureWindow(RootWnd *window, const char *bitmap) {
  if (window == NULL) {
    ODS("illegal param", "window == NULL");
    return;
  }
  BaseTexture *s = new BaseTexture(window, bitmap);
  baseTextureList.addItem(s);
}

void WndApi::skin_unregisterBaseTextureWindow(RootWnd *window) {
  if (window == NULL) {
    ODS("illegal param", "window == NULL");
    return;
  }
  for (int i=0;i<baseTextureList.getNumItems();i++) {
    if (baseTextureList.enumItem(i)->getWnd() == window) {
      BaseTexture *s = baseTextureList.enumItem(i);
      baseTextureList.delByPos(i);
      delete s;
      if (baseTextureList.getNumItems() == 0)
        baseTextureList.removeAll();
      break;
    }
  }
}

void WndApi::appdeactivation_push_disallow(RootWnd *from_whom) {
  AppDeactivationMgr::push_disallow(from_whom);
}

void WndApi::appdeactivation_pop_disallow(RootWnd *from_whom) {
  AppDeactivationMgr::pop_disallow(from_whom);
}

int WndApi::appdeactivation_isallowed(RootWnd *w) {
  return AppDeactivationMgr::is_deactivation_allowed(w);
}

void WndApi::appdeactivation_setbypass(int i) {
  AppDeactivationMgr::setbypass(i);
}

#ifdef WASABI_COMPILE_PAINTSETS
#ifdef WASABI_COMPILE_IMGLDR

void WndApi::paintset_render(int set, CanvasBase *c, const RECT *r, int alpha) {
  if (c == NULL) {
    ODS("illegal param", "c == NULL");
    return;
  }
  if (r == NULL) {
    ODS("illegal param", "r == NULL");
    return;
  }
  paintset_renderPaintSet(set, c, r, alpha);
}

#ifdef WASABI_COMPILE_FONTS
void WndApi::paintset_renderTitle(char *t, CanvasBase *c, const RECT *r, int alpha) {
  if (c == NULL) {
    ODS("illegal param", "c == NULL");
    return;
  }
  if (r == NULL) {
    ODS("illegal param", "r == NULL");
    return;
  }
  ::paintset_renderTitle(t, c, r, alpha);
}
#endif // WASABI_COMPILE_FONTS
#endif // WASABI_COMPILE_IMGLDR
#endif // WASABI_COMPILE_PAINTSETS

BaseTexture *WndApi::getBaseTexture(RootWnd *b) {
  if (b == NULL) return NULL;
  for (int i=0;i<baseTextureList.getNumItems();i++)
    if (baseTextureList.enumItem(i)->getWnd() == b)
      return baseTextureList.enumItem(i);
  return NULL;
}

void WndApi::renderBaseTexture(RootWnd *base, CanvasBase *c, const RECT &r, RootWnd *dest, int alpha) {
  renderBaseTexture(base, getBaseTexture(base), c, r, dest, alpha);
}

void WndApi::renderBaseTexture(RootWnd *base, BaseTexture *bt, CanvasBase *c, const RECT &r, RootWnd *dest, int alpha) {
  if (!bt) {
    DebugString("Warning, no base texture found in renderBaseTexture\n");
    return;
  }

  bt->renderBaseTexture(base, c, r, dest, alpha);
}

#ifndef SAFEROUND
#define SAFEROUND(d) ((float)(int)d == d) ? (int)d : (d - (float)(int)d > 0) ? ((int)d)+1 : ((int)d)-1
#endif

void BaseTexture::renderBaseTexture(RootWnd *wndbase, CanvasBase *c, const RECT &r, RootWnd *dest, int alpha) {

  // pick our basetexture
  AutoSkinBitmap *b = &texture;

  if(!b) return;

  // srcRect is the source rectangle in the basetexture
  RECT srcRect; 
  // destProjectedRect is the basetexture rectangle projected to dest coordinates
  RECT destProjectedRect; 
  
  RootWnd *p = dest;
  POINT pt;
  int sx=0, sy=0;
  while (p && p != wndbase) {
    if (!p->isVirtual()) {
      p->getPosition(&pt);
      sx += pt.x;
      sy += pt.y;
    }
    p = p->getParent();
  }
  ASSERT(p);  

  wndbase->getNonClientRect(&destProjectedRect);
  destProjectedRect.left -= sx;
  destProjectedRect.top -= sy;
  destProjectedRect.right -= sx;
  destProjectedRect.bottom -= sy;

  srcRect.left = 0;
  srcRect.top = 0;
  srcRect.right = b->getWidth();
  srcRect.bottom = b->getHeight();

#if 0
  // NONPORTABLE 
  HDC hdc=c->getHDC();
  HRGN oldRgn=CreateRectRgn(0,0,0,0);
  HRGN newRgn=CreateRectRgnIndirect(&r);

  int cs=GetClipRgn(hdc,oldRgn);

  ExtSelectClipRgn(hdc,newRgn,(cs!=1)?RGN_COPY:RGN_AND);
#endif
  BaseCloneCanvas clone( c );
  RegionI oldRgn;
  RegionI newRgn( &r );

  int cs = clone.getClipRgn( &oldRgn );

  if ( cs ) newRgn.andRegion( &oldRgn );

  clone.selectClipRgn( &newRgn );

  b->stretchToRectAlpha(c, &srcRect, &destProjectedRect, alpha);

  clone.selectClipRgn(cs ? &oldRgn : NULL);
}

int WndApi::forwardOnMouseWheel(int l, int a) {
  return 0;
}

int WndApi::paintset_present(int set) {
  return ::paintset_present(set);
}

void WndApi::setDefaultDropTarget(void *dt) {
  default_drop_target = dt;
}

void *WndApi::getDefaultDropTarget() {
  return default_drop_target;
}

int WndApi::pushKeyboardLock() {
  return ++kbdlock;
}

int WndApi::popKeyboardLock() {
  return --kbdlock;
}

int WndApi::isKeyboardLocked() {
  return kbdlock;
}


RootWnd *WndApi::genericwnd = NULL;
PtrList<BaseTexture> WndApi::baseTextureList;
void *WndApi::default_drop_target = NULL;
int WndApi::kbdlock = 0;
