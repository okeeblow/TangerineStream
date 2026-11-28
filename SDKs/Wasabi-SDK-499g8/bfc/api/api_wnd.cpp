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
#include "api_wnd.h"

#define CBCLASS wnd_apiI
START_DISPATCH;
VCB(API_WND_SETROOTWND,                     main_setRootWnd);
CB(API_WND_GETROOTWND,                      main_getRootWnd);
CB(API_WND_GETMODALWND,                     getModalWnd);
VCB(API_WND_PUSHMODALWND,                   popModalWnd);
VCB(API_WND_POPMODALWND,                    popModalWnd);
CB(API_WND_ROOTWNDFROMPOINT,                rootWndFromPoint);
VCB(API_WND_REGISTERROOTWND,                registerRootWnd);
VCB(API_WND_UNREGISTERROOTWND,              unregisterRootWnd);
CB(API_WND_ROOTWNDISVALID,                  rootwndIsValid);
CB(API_WND_INTERCEPTONCHAR,                 interceptOnChar);
CB(API_WND_INTERCEPTONKEYDOWN,              interceptOnKeyDown);
CB(API_WND_INTERCEPTONKEYUP,                interceptOnKeyUp);
CB(API_WND_INTERCEPTONSYSKEYDOWN,           interceptOnSysKeyDown);
CB(API_WND_INTERCEPTONSYSKEYUP,             interceptOnSysKeyUp);
VCB(API_WND_HOOKKEYBOARD,                   hookKeyboard);
VCB(API_WND_UNHOOKKEYBOARD,                 unhookKeyboard);
VCB(API_WND_KBDRESET,                       kbdReset);
CB(API_WND_FORWARDONCHAR,                   forwardOnChar);
CB(API_WND_FORWARDONKEYDOWN,                forwardOnKeyDown);
CB(API_WND_FORWARDONKEYUP,                  forwardOnKeyUp);
CB(API_WND_FORWARDONSYSKEYDOWN,             forwardOnSysKeyDown);
CB(API_WND_FORWARDONSYSKEYUP,               forwardOnSysKeyUp);
CB(API_WND_FORWARDONKILLFOCUS,              forwardOnKillFocus);
CB(API_WND_POPUPEXIT_CHECK,                 popupexit_check);
VCB(API_WND_POPUPEXIT_SIGNAL,               popupexit_signal);
VCB(API_WND_POPUPEXIT_REGISTER,             popupexit_register);
VCB(API_WND_POPUPEXIT_DEREGISTER,           popupexit_deregister);
VCB(API_WND_RENDERBASETEXTURE,              skin_renderBaseTexture);
VCB(API_WND_REGISTERBASETEXTUREWINDOW,      skin_registerBaseTextureWindow);
VCB(API_WND_UNREGISTERBASETEXTUREWINDOW,    skin_unregisterBaseTextureWindow);
VCB(API_WND_APPDEACTIVATION_PUSH_DISALLOW,  appdeactivation_push_disallow);
VCB(API_WND_APPDEACTIVATION_POP_DISALLOW,   appdeactivation_pop_disallow);
CB(API_WND_APPDEACTIVATION_ISALLOWED,       appdeactivation_isallowed);
VCB(API_WND_APPDEACTIVATION_SETBYPASS,      appdeactivation_setbypass);
CB(API_WND_FORWARDONMOUSEWHEEL,             forwardOnMouseWheel);
#ifdef WASABI_COMPILE_PAINTSETS
CB(API_WND_PAINTSET_PRESENT,                paintset_present );
#ifdef WASABI_COMPILE_IMGLDR
VCB(API_WND_PAINTSET_RENDER,                paintset_render);
#ifdef WASABI_COMPILE_FONTS
VCB(API_WND_PAINTSET_RENDERTITLE,           paintset_renderTitle);
#endif // fonts
#endif // imgldr
#endif // paintsets
// fg> this may need to go away eventually but i need it _right now_
VCB(API_WND_SETDEFAULTDROPTARGET,           setDefaultDropTarget);
CB(API_WND_GETDEFAULTDROPTARGET,            getDefaultDropTarget);
CB(API_WND_PUSHKBDLOCK,                     pushKeyboardLock);
CB(API_WND_POPKBDLOCK,                      popKeyboardLock);
CB(API_WND_ISKBDLOCKED,                     isKeyboardLocked);
END_DISPATCH;
