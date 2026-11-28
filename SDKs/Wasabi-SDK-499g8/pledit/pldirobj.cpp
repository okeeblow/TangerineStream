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
#include "pldirobj.h"
#include "../studio/api.h"

#include "pldir.h"

static PlDirScriptObjectController _pldirController;
ScriptObjectController *pldirController = &_pldirController;

// -----------------------------------------------------------------------------------------------------
// Service

ScriptObjectController *PlDirScriptObjectSvc::getController(int n) {
  switch (n) {
    case 0:
      return pldirController;
  }
  return NULL;
}

// -----------------------------------------------------------------------------------------------------
// PlDirObject

PlDirObject::PlDirObject() {
  getScriptObject()->vcpu_setInterface(PLDIR_SCRIPTOBJECT_GUID, (void *)static_cast<PlDirObject *>(this));
  getScriptObject()->vcpu_setClassName("PlDir");
  getScriptObject()->vcpu_setController(pldirController);
}

PlDirObject::~PlDirObject() {
}

void PlDirObject::showCurrentlyPlayingEntry() {
//  PlDir *pld = PlDir::getPlDir();
//  if (pld != NULL) pld->showCurrentlyPlayingEntry();
}

// -----------------------------------------------------------------------------------------------------
// PlDirScriptObjectController

function_descriptor_struct PlDirScriptObjectController::exportedFunction[] = {
  {"showCurrentlyPlayingEntry",           0, (void*)PlDirScriptObjectController::pldir_showCurrentlyPlayingEntry},
};

PlDirScriptObjectController::getNumFunctions() {
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

ScriptObject *PlDirScriptObjectController::instantiate() {
  PlDirObject *c = new PlDirObject;
  if (!c) return NULL;
  return c->getScriptObject();
}

void PlDirScriptObjectController::destroy(ScriptObject *o) {
  PlDirObject *obj = static_cast<PlDirObject *>(o->vcpu_getInterface(PLDIR_SCRIPTOBJECT_GUID));
  ASSERT(obj != NULL);
  delete obj;
}

void *PlDirScriptObjectController::encapsulate(ScriptObject *o) {
  return NULL; // nobody can inherits from me yet (see rootobj or guiobj in studio if you want to allow that)
}

void PlDirScriptObjectController::deencapsulate(void *) {
}

// Script calls

scriptVar PlDirScriptObjectController::pldir_showCurrentlyPlayingEntry(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT  
  
/*

  // For non static classes :

  ASSERT(o != NULL);
  PlDirObject *pldo = static_cast<PlDirObject *>(o->vcpu_getInterface(PLDIR_SCRIPTOBJECT_GUID));
  if (pldo)
    pldo->doSomething(GET_SCRIPT_STRING(blah));
  
*/

  // For static classes :

  PlDirObject::showCurrentlyPlayingEntry();

  RETURN_SCRIPT_VOID;
}
