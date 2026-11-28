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
#include "xuipathpicker.h"

// -----------------------------------------------------------------------
char PathPickerXuiObjectStr[] = "Wasabi:PathPicker"; 
char PathPickerXuiSvcName[] = "Wasabi:PathPicker XuiObject Service"; 

// -----------------------------------------------------------------------
ScriptPathPicker::ScriptPathPicker() {
  getScriptObject()->vcpu_setInterface(pathPickerGuid, (void *)static_cast<PathPicker *>(this));
  getScriptObject()->vcpu_setClassName("PathPicker"); // this is the script class name
  getScriptObject()->vcpu_setController(pathPickerController);
}

// -----------------------------------------------------------------------
ScriptPathPicker::~ScriptPathPicker() {
}

// -----------------------------------------------------------------------
// Script Object

PathPickerScriptController _pathPickerController;
PathPickerScriptController *pathPickerController = &_pathPickerController;

// -- Functions table -------------------------------------
function_descriptor_struct PathPickerScriptController::exportedFunction[] = {
  {"getPath",            0, (void*)PathPickerScriptController::PathPicker_getPath},
  {"onPathChanged",      1, (void*)PathPickerScriptController::PathPicker_onPathChanged},
};
                                      
ScriptObject *PathPickerScriptController::instantiate() {
  ScriptPathPicker *sddl = new ScriptPathPicker;
  ASSERT(sddl != NULL);
  return sddl->getScriptObject();
}

void PathPickerScriptController::destroy(ScriptObject *o) {
  ScriptPathPicker *sddl= static_cast<ScriptPathPicker *>(o->vcpu_getInterface(pathPickerGuid));
  ASSERT(sddl != NULL);
  delete sddl;
}

void *PathPickerScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation 
}

void PathPickerScriptController::deencapsulate(void *o) {
}

int PathPickerScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *PathPickerScriptController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar PathPickerScriptController::PathPicker_getPath(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptPathPicker *sddl = static_cast<ScriptPathPicker*>(o->vcpu_getInterface(pathPickerGuid));
  const char *p="";
  if (sddl) p = sddl->getPath();
  return MAKE_SCRIPT_STRING(p);
}

//   PathPickerScriptController::PathPicker_onNewPath(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_STRING(newpath));
scriptVar PathPickerScriptController::PathPicker_onPathChanged(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar newpath) {
  SCRIPT_FUNCTION_INIT; 
  PROCESS_HOOKS1(o, pathPickerController, newpath);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, newpath);
}
