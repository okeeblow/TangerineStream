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
#ifndef __MENUSURFACE_H
#define __MENUSURFACE_H

#include "../bfc/dispatch.h"
#include "guiobjwnd.h"

#define  MenuButtonSurface_PARENT GuiObjectWnd 

class MenuButtonSurface : public Dispatchable {
  public:

    const char *getParentMenuId();
    void openParentMenu();
    void closeParentMenu();
    RootWnd *getParentWnd();
    void setParentWnd(RootWnd *w);

  enum {
    GETPARENTMENUID = 10,
    OPENPARENTMENU  = 20,
    CLOSEPARENTMENU = 30,
    GETPARENTWND    = 40,
    SETPARENTWND    = 50,
  };
};

inline const char *MenuButtonSurface::getParentMenuId() {
  return _call(GETPARENTMENUID, (const char *)NULL);
}

inline void MenuButtonSurface::openParentMenu() {
  _voidcall(OPENPARENTMENU);
}

inline void MenuButtonSurface::closeParentMenu() {
  _voidcall(CLOSEPARENTMENU);
}

inline RootWnd *MenuButtonSurface::getParentWnd() {
  return _call(GETPARENTWND, (RootWnd *)NULL);
}

inline void MenuButtonSurface::setParentWnd(RootWnd *w) {
  _voidcall(SETPARENTWND, w);
}


#endif