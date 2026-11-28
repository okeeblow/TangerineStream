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

#include "svc_accroleserver.h"
#include "../../common/script/guiobject.h"
#include "../../bfc/rootwnd.h"

#define CBCLASS svc_accRoleServerI
START_DISPATCH;
  CB(RS_HANDLEROLE,      handleRole);
  CB(RS_CREATEOBJECT,    createObject);
  VCB(RS_DESTROYOBJECT,  destroyObject);
END_DISPATCH;
#undef CBCLASS


#define CBCLASS roleServerObjectI
START_DISPATCH;
  CB(RSO_WNDPROC,        wndProc);
  CB(RSO_GETHWND,        gethWnd);
  CB(RSO_FLATTENCONTENT, flattenContent);
END_DISPATCH;
#undef CBCLASS


roleServerObjectI::roleServerObjectI(HWND par, RootWnd *w) {
  wnd = w;
  hwnd = NULL;
  parent = par;
  triedyet = 0;
}

roleServerObjectI::~roleServerObjectI() {
  if (hwnd != NULL)
    DestroyWindow(hwnd);
}

RootWnd *roleServerObjectI::getWnd() {
  return wnd;
}

HWND roleServerObjectI::gethWnd() {
  if (!triedyet) {
    triedyet = 1;
    hwnd = createWindow(parent);
    if (hwnd !=NULL)
      oldproc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
    else
      oldproc = NULL;
  }
  return hwnd;
}

ScriptObject *roleServerObjectI::getScriptObject() {
  if (wnd == NULL) return NULL;
  GuiObject *go = wnd->getGuiObject();
  if (go == NULL) return NULL;
  return go->guiobject_getScriptObject();
}

WNDPROC roleServerObjectI::getOldProc() {
  return oldproc;
}

int roleServerObjectI::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return CallWindowProc(oldproc, hWnd, uMsg, wParam, lParam);
}

int roleServerObjectI::flattenContent(HWND *w) {
  return FLATTENFLAG_ASKPARENT;
}
