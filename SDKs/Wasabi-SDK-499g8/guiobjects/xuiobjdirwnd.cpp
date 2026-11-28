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

#include "xuiobjdirwnd.h"

enum {
  SCRIPTOBJDIRWND_DIR=10,
  SCRIPTOBJDIRWND_ACTION_TARGET=20,
  DISPLAYTARGET,
  DEFAULTDISPLAY,
  FORCEVIRTUAL,
};

char ScriptObjDirWndXuiObjectStr[] = "ObjDirView"; // This is the xml tag
char ScriptObjDirWndXuiSvcName[] = "ObjDirView XuiObject Service";

ScriptObjDirWnd::ScriptObjDirWnd() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "dir", SCRIPTOBJDIRWND_DIR, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "target", SCRIPTOBJDIRWND_ACTION_TARGET, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "displaytarget", DISPLAYTARGET, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "defaultdisplay", DEFAULTDISPLAY, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "forcevirtual", FORCEVIRTUAL, XUI_ATTRIBUTE_IMPLIED);
}

ScriptObjDirWnd::~ScriptObjDirWnd() { }

int ScriptObjDirWnd::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTOBJDIRWND_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);
  switch (xmlattributeid) {
    case SCRIPTOBJDIRWND_DIR:
      setTargetDirName(value);
    break;
    case SCRIPTOBJDIRWND_ACTION_TARGET:
      setActionTarget(value);
    break;
    case DISPLAYTARGET:
      setDisplayTarget(value);
    break;
    case DEFAULTDISPLAY:
      setDefaultDisplay(value);
    break;
    case FORCEVIRTUAL:
      setVirtual(ATOI(value));
    break;
    default:
      return 0;
  }
  return 1;
}

