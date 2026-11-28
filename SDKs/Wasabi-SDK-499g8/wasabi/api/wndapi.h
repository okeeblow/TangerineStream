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
#include "../../bfc/api/api_wnd.h"
#include "../../bfc/ptrlist.h"
#include "../../bfc/autobitmap.h"

// ---

class BaseTexture {
  public:
    BaseTexture(RootWnd *_wnd, const char *_bmp) : wnd(_wnd), texture(_bmp) {}
    virtual ~BaseTexture() {}

    SkinBitmap *getTexture() { return texture.getBitmap(); }
    RootWnd *getWnd() { return wnd; }

    virtual void renderBaseTexture(RootWnd *wndbase, CanvasBase *c, const RECT &r, RootWnd *dest, int alpha);

  private:
    RootWnd *wnd;
    AutoSkinBitmap texture;
};

// ---

class WndApi : public wnd_apiI {

  public:

    WndApi();
    virtual ~WndApi();

    virtual RootWnd *main_getRootWnd();
    virtual void main_setRootWnd(RootWnd *w);
    virtual RootWnd *getModalWnd();
    virtual void pushModalWnd(RootWnd *w=MODALWND_NOWND);
    virtual void popModalWnd(RootWnd *w=MODALWND_NOWND);
    virtual RootWnd *rootWndFromPoint(POINT *pt);
    virtual void registerRootWnd(RootWnd *wnd);
    virtual void unregisterRootWnd(RootWnd *wnd);
    virtual int rootwndIsValid(RootWnd *wnd);
    virtual void hookKeyboard(RootWnd *hooker);
    virtual void unhookKeyboard(RootWnd *hooker);
    virtual void kbdReset();
    virtual int interceptOnChar(unsigned int c);
    virtual int interceptOnKeyDown(int k);
    virtual int interceptOnKeyUp(int k);
    virtual int interceptOnSysKeyDown(int k, int kd);
    virtual int interceptOnSysKeyUp(int k, int kd);
    virtual int forwardOnChar(RootWnd *from, unsigned int c, int kd);
    virtual int forwardOnKeyDown(RootWnd *from, int k, int kd);
    virtual int forwardOnKeyUp(RootWnd *from, int k, int kd);
    virtual int forwardOnSysKeyDown(RootWnd *from, int k, int kd);
    virtual int forwardOnSysKeyUp(RootWnd *from, int k, int kd);
    virtual int forwardOnKillFocus();
    virtual void popupexit_deregister(PopupExitCallback *cb);
    virtual void popupexit_register(PopupExitCallback *cb, RootWnd *watched);
    virtual int popupexit_check(RootWnd *w);
    virtual void popupexit_signal();
    virtual void skin_renderBaseTexture(RootWnd *base, CanvasBase *c, const RECT *r, RootWnd *destWnd, int alpha=255);
    virtual void skin_registerBaseTextureWindow(RootWnd *window, const char *bmp=NULL);
    virtual void skin_unregisterBaseTextureWindow(RootWnd *window);
    virtual void appdeactivation_push_disallow(RootWnd *w);
    virtual void appdeactivation_pop_disallow(RootWnd *w);
    virtual int appdeactivation_isallowed(RootWnd *w);
    virtual void appdeactivation_setbypass(int i);
#ifdef WASABI_COMPILE_PAINTSETS
    virtual int paintset_present(int set);
#ifdef WASABI_COMPILE_IMGLDR
    virtual void paintset_render(int set, CanvasBase *c, const RECT *r, int alpha=255);
#ifdef WASABI_COMPILE_FONTS
    virtual void paintset_renderTitle(char *t, CanvasBase *c, const RECT *r, int alpha=255);
#endif // fonts
#endif // imgldr
#endif // paintsets
    virtual int forwardOnMouseWheel(int l, int a);
    virtual void setDefaultDropTarget(void *dt);
    virtual void *getDefaultDropTarget();
    static int getNumBaseTextures() { return baseTextureList.getNumItems(); }
    virtual int pushKeyboardLock();
    virtual int popKeyboardLock();
    virtual int isKeyboardLocked();

  private:

    static BaseTexture *getBaseTexture(RootWnd *b);
    static void renderBaseTexture(RootWnd *base, CanvasBase *c, const RECT &r, RootWnd *dest, int alpha);
    static void renderBaseTexture(RootWnd *base, BaseTexture *s, CanvasBase *c, const RECT &r, RootWnd *dest, int alpha);
    static PtrList<BaseTexture> baseTextureList;
    static RootWnd *genericwnd;
    static void *default_drop_target;
    static int kbdlock;
};

extern WndApi _wndApi;
