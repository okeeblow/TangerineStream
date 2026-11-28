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
//<?#include "<class data="implementationheader"/>"
#include "rootobjcontroller.h"
//?>

#include "rootobj.h"
#include "studio/script/rootobjecti.h"

RootScriptObjectController _rootScriptObjectController;
RootScriptObjectController *rootScriptObjectController = &_rootScriptObjectController;

// -- Functions table -------------------------------------
function_descriptor_struct RootScriptObjectController::exportedFunction[] = {
  {"getClassName",  0, (void*)RootObject_ScriptMethods::getClassName },
  {"notify",        4, (void*)RootObject_ScriptMethods::notify },
  {"onNotify",      4, (void*)RootObject_ScriptMethods::onNotify },
};

const char *RootScriptObjectController::getClassName() {
  return "Object";
}

const char *RootScriptObjectController::getAncestorClassName() {
  return NULL;
}

int RootScriptObjectController::getNumFunctions() {
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *RootScriptObjectController::getExportedFunctions() {
  return exportedFunction;
}

GUID RootScriptObjectController::getClassGuid() {
  return rootObjectGuid;
}

ScriptObject *RootScriptObjectController::instantiate() {
  RootObjectInstance *o = new RootObjectInstance;
  ASSERT(o != NULL);
  return o->getScriptObject();
}

void RootScriptObjectController::destroy(ScriptObject *o) {
  RootObjectInstance *obj = static_cast<RootObjectInstance *>(o->vcpu_getInterface(rootObjectGuid)); 
  ASSERT(obj != NULL);
  delete obj;
}

void RootScriptObjectController::deencapsulate(void *o) {
  RootObjectI *obj = static_cast<RootObjectI *>(o); 
  delete obj;
}

void *RootScriptObjectController::encapsulate(ScriptObject *o) {
  return new RootObjectI(o);
}

// -------------------------------------------------------------------------------------------------------------------------------------
// Script Methods and Events for RootObject (Object)
// -------------------------------------------------------------------------------------------------------------------------------------

scriptVar RootObject_ScriptMethods::getClassName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT; 
  return MAKE_SCRIPT_STRING(o->vcpu_getClassName());
} 

scriptVar RootObject_ScriptMethods::onNotify(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar s, scriptVar s2, scriptVar i1, scriptVar i2) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS4(o, rootScriptObjectController, s, s2, i1, i2);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT4(o, s, s2, i1, i2);
}

scriptVar RootObject_ScriptMethods::notify(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar s, scriptVar s2, scriptVar i1, scriptVar i2) {
  SCRIPT_FUNCTION_INIT
  RootObject *ro = static_cast<RootObject*>(o->vcpu_getInterface(rootObjectGuid));
  if (ro) ro->rootobject_notify(GET_SCRIPT_STRING(s), GET_SCRIPT_STRING(s2), GET_SCRIPT_INT(i1), GET_SCRIPT_INT(i2));
  RETURN_SCRIPT_VOID;
}
