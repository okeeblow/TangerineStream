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
#include "xuistatus.h"

char StatusXuiObjectStr[] = "LayoutStatus"; // This is the xml tag
char StatusXuiSvcName[] = "Status XuiObject Service";

LayoutStatusController _layoutStatusController;
LayoutStatusController *layoutStatusController = &_layoutStatusController;

enum {
  EXCLUDE, INCLUDE
};

XuiStatus::XuiStatus() {
  getScriptObject()->vcpu_setInterface(layoutStatusGuid, (void *)static_cast<XuiStatus *>(this));
  getScriptObject()->vcpu_setClassName("LayoutStatus"); // this is the script class name
  getScriptObject()->vcpu_setController(layoutStatusController);

  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "exclude", EXCLUDE, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "includeonly", INCLUDE, XUI_ATTRIBUTE_IMPLIED);
}

int XuiStatus::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return XUISTATUS_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);
  switch (xmlattributeid) {
    case EXCLUDE:
      setExclude(value);
    break;
    case INCLUDE:
      setIncludeOnly(value);
    break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void XuiStatus::callme(const char *str) {
  fakeButtonPush(str);
}

// -----------------------------------------------------------------------
// Script Object

// -- Functions table -------------------------------------
function_descriptor_struct LayoutStatusController::exportedFunction[] = {
  {"callme",             1, (void*)LayoutStatusController::layoutstatus_callme },
};

ScriptObject *LayoutStatusController::instantiate() {
  XuiStatus *xs = new XuiStatus;
  ASSERT(xs != NULL);
  return xs->getScriptObject();
}

void LayoutStatusController::destroy(ScriptObject *o) {
  XuiStatus *xs = static_cast<XuiStatus *>(o->vcpu_getInterface(layoutStatusGuid));
  ASSERT(xs!= NULL);
  delete xs;
}

void *LayoutStatusController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for layoutstatus yet
}

void LayoutStatusController::deencapsulate(void *o) {
}

int LayoutStatusController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *LayoutStatusController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar LayoutStatusController::layoutstatus_callme(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar str) {
  SCRIPT_FUNCTION_INIT
  XuiStatus *xs = static_cast<XuiStatus *>(o->vcpu_getInterface(layoutStatusGuid));
  if (xs) xs->callme(GET_SCRIPT_STRING(str));
  RETURN_SCRIPT_VOID;  
}

