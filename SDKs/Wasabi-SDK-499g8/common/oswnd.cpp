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
#include "oswnd.h"
#include "../bfc/canvas.h"

int OSWnd::onInit() {
  OSWND_PARENT::onInit();
  onSetVisible(isVisible());
  return 1;
}

void OSWnd::onSetVisible(int show) {
#ifdef WIN32
  ShowWindow(getOSHandle(), show ? SW_NORMAL : SW_HIDE);
#endif
}

int OSWnd::onResize() {
  OSWND_PARENT::onResize();
#ifdef WIN32
  if (getOSHandle()) {
    RECT r;
    getClientRect(&r);
    SetWindowPos(getOSHandle(), NULL, r.left, r.top, r.right-r.left,r.bottom-r.top, SWP_NOZORDER|SWP_NOACTIVATE);
  }
#endif
  return 1;
}


