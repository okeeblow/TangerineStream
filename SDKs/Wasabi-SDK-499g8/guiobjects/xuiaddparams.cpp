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

#include "xuiaddparams.h"
#include "../common/script/scriptguid.h"

// -----------------------------------------------------------------------
char AddParamsXuiObjectStr[] = "AddParams"; // This is the xml tag
char AddParamsXuiSvcName[] = "AddParams XuiObject Service"; 

// -----------------------------------------------------------------------
AddParams::AddParams() {
}

// -----------------------------------------------------------------------
AddParams::~AddParams() {
  pastlist.deleteAll();
}

// -----------------------------------------------------------------------
int AddParams::setXmlParam(const char *param, const char *value) { 
  int r = ADDPARAMS_PARENT::setXmlParam(param, value);
  if (!STRCASEEQLSAFE(param, "group") && !STRCASEEQLSAFE(param, "target")) {
    Pair<String, String> *pair = new Pair<String, String>(param, value);
    pastlist.addItem(pair);
  }
  return r;
}

// -----------------------------------------------------------------------
void AddParams::actuator_onPerform(GuiObject *target) { // guaranteed non NULL
  ADDPARAMS_PARENT::actuator_onPerform(target);
  XmlObject *xtarget = static_cast<XmlObject *>(target->guiobject_getScriptObject()->vcpu_getInterface(xmlObjectGuid));
  foreach(pastlist)
    const char *a = pastlist.getfor()->a;
    int xp = xtarget->getXmlParam(a);
    String newval((xp == -1) ? "" : xtarget->getXmlParamValue(xp));
    newval.cat(pastlist.getfor()->b);
    xtarget->setXmlParam(a, newval);
  endfor;
}
