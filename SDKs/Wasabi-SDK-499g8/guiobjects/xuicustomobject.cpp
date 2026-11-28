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
#include "xuicustomobject.h"
#include "../bfc/notifmsg.h"

// -----------------------------------------------------------------------
char CustomObjectXuiObjectStr[] = "CustomObject"; // This is the xml tag
char CustomObjectXuiSvcName[] = "CustomObject XuiObject Service"; 

// -----------------------------------------------------------------------
XuiCustomObject::XuiCustomObject() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "groupid", CUSTOMOBJECT_SETGROUP, XUI_ATTRIBUTE_IMPLIED);

  ScriptObject *o = getGuiObject()->guiobject_getScriptObject();
  o->vcpu_setInterface(customObjectGuid, static_cast<CustomObject *>(this));

}

// -----------------------------------------------------------------------
XuiCustomObject::~XuiCustomObject() {
}

// -----------------------------------------------------------------------
int XuiCustomObject::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return CUSTOMOBJECT_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case  CUSTOMOBJECT_SETGROUP:
      setContent(value);
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void XuiCustomObject::setGroup(const char *elementname) {
  setContent(elementname);
}

// -----------------------------------------------------------------------
void XuiCustomObject::customobject_setRootWnd(RootWnd *w) {
  rootwndholder_setRootWnd(NULL);
  groupid = "";
  setContent(groupid);
  if (w != NULL) rootwndholder_setRootWnd(w);
  notifyParent(ChildNotify::AUTOWHCHANGED);
}

RootWnd *XuiCustomObject::customobject_getRootWnd() {
  return rootwndholder_getRootWnd();
}
