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
#include "xuihideobject.h"

// -----------------------------------------------------------------------
char HideObjectXuiObjectStr[] = "HideObject"; // This is the xml tag
char HideObjectXuiSvcName[] = "HideObjct XuiObject Service"; 

// -----------------------------------------------------------------------
HideObject::HideObject() : HIDEOBJECT_PARENT() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "hide",      HIDEOBJECT_HIDE,      XUI_ATTRIBUTE_IMPLIED);
}


// -----------------------------------------------------------------------
int HideObject::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return HIDEOBJECT_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case HIDEOBJECT_HIDE:
      actuator_setTarget(value);
      break;
    default:
      return 0;
  }
  return 1; 
}

// -----------------------------------------------------------------------
void HideObject::actuator_onPerform(GuiObject *target) { // guaranteed non NULL
  RootWnd *w = target->guiobject_getRootWnd();
  if (w != NULL) {
    w->setVisible(0);
  }
}


