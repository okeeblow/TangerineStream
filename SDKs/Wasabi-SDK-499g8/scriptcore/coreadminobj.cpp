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
#include "../studio/api.h"
#include "../studio/services/svc_coreadmin.h"
#include "scriptcore.h"
#include "coreadminobj.h"
#include "coreobj.h"

static CoreAdminScriptObjectController _coreAdminController;
ScriptObjectController *coreAdminController = &_coreAdminController;

// Table of exported functions and events
// "function name",  n. params, function_pointer
function_descriptor_struct CoreAdminScriptObjectController::exportedFunction[] = {
  {"getNamedCore",         1, (void*)coreadmin_getNamedCore},
  {"newNamedCore",         1, (void*)coreadmin_newNamedCore},
  {"freeCore",             1, (void*)coreadmin_freeCore},
  {"freeCoreByName",       1, (void*)coreadmin_freeCoreByName},
};

CoreAdminScriptObjectController::getNumFunctions() {
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

ScriptObject *CoreAdminScriptObjectController::instantiate() {
  ScriptCoreAdminObject *scao = new ScriptCoreAdminObject;
  ASSERT(scao != NULL);
  return scao->getScriptObject();
}

void CoreAdminScriptObjectController::destroy(ScriptObject *o) {
  ScriptCoreAdminObject *scao = static_cast<ScriptCoreAdminObject *>(o->vcpu_getInterface(COREADMIN_SCRIPTOBJECT_GUID));
  ASSERT(scao != NULL);
  delete scao;
}

void *CoreAdminScriptObjectController::encapsulate(ScriptObject *o) {
  return NULL;
}

void CoreAdminScriptObjectController::deencapsulate(void *o) {
}

scriptVar CoreAdminScriptObjectController::coreadmin_getNamedCore(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar name) {
  SCRIPT_FUNCTION_INIT  
  ScriptCoreAdminObject *scao = static_cast<ScriptCoreAdminObject *>(o->vcpu_getInterface(COREADMIN_SCRIPTOBJECT_GUID));
  if (scao) {
    const char *cname = GET_SCRIPT_STRING(name);
    if (cname) {
#ifndef FAKE_SCRIPTCORE
      // Get the token for the requested core.
      CoreToken core = scao->getNamedCore(cname);
      if (core == -1) {
        RETURN_SCRIPT_VOID;
      }
#endif
      // Make a new Core scriptobj to point to the requested core.
      CoreScriptObjectController *controller = static_cast<CoreScriptObjectController *>(coreController);
#ifndef FAKE_SCRIPTCORE
      ScriptObject *coreobj = controller->instantiate(core);
#else
      ScriptObject *coreobj = controller->instantiate(NULL);
#endif
      return MAKE_SCRIPT_OBJECT(coreobj);
    }
  }
  RETURN_SCRIPT_NULL;
}

scriptVar CoreAdminScriptObjectController::coreadmin_newNamedCore(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar name) {
  SCRIPT_FUNCTION_INIT  
  ScriptCoreAdminObject *scao = static_cast<ScriptCoreAdminObject *>(o->vcpu_getInterface(COREADMIN_SCRIPTOBJECT_GUID));
  if (scao) {
    const char *cname = GET_SCRIPT_STRING(name);
    if (cname) {
      // Get the token for the created core.
      CoreToken core = scao->newNamedCore(cname);
      // Make a new Core scriptobj to point to the created core.
      CoreScriptObjectController *controller = static_cast<CoreScriptObjectController *>(coreController);
#ifndef FAKE_SCRIPTCORE
      ScriptObject *coreobj = controller->instantiate(core);
      // Then get the object back and bind a sequencer and stuff to it.  Like we just said "new Core"
      ScriptCoreObject *sco = static_cast<ScriptCoreObject *>(coreobj->vcpu_getInterface(CORE_SCRIPTOBJECT_GUID));
      if (sco) {
        sco->initAsCreated();
      }
#else
      ScriptObject *coreobj = controller->instantiate(NULL);
#endif
      return MAKE_SCRIPT_OBJECT(coreobj);
    }
  }
  RETURN_SCRIPT_VOID;
}

scriptVar CoreAdminScriptObjectController::coreadmin_freeCore(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar core) {
  SCRIPT_FUNCTION_INIT  
  ScriptCoreAdminObject *scao = static_cast<ScriptCoreAdminObject *>(o->vcpu_getInterface(COREADMIN_SCRIPTOBJECT_GUID));
  if (scao) {
    // Pull the ScriptCoreObject from the scriptVar
    ScriptObject *coreobj = GET_SCRIPT_OBJECT(core);
    if (coreobj) {
      ScriptCoreObject *sco = static_cast<ScriptCoreObject *>(coreobj->vcpu_getInterface(CORE_SCRIPTOBJECT_GUID));
      if (sco) {
#ifndef FAKE_SCRIPTCORE
        scao->freeCore(sco->core_handle);
#endif
        return MAKE_SCRIPT_INT(1);
      }
    }
  }
  return MAKE_SCRIPT_INT(0);
}

scriptVar CoreAdminScriptObjectController::coreadmin_freeCoreByName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar name) {
  SCRIPT_FUNCTION_INIT  
  ScriptCoreAdminObject *scao = static_cast<ScriptCoreAdminObject *>(o->vcpu_getInterface(COREADMIN_SCRIPTOBJECT_GUID));
  if (scao) {
    const char *cname = GET_SCRIPT_STRING(name);
    if (cname) {
      // Try to remove it.
      int retval = scao->freeCoreByName(cname);
      return MAKE_SCRIPT_INT(retval);
    }
  }
  return MAKE_SCRIPT_INT(0);
}

                        
// -----------------------------------------------------------------------------------------------------

ScriptCoreAdminObject::ScriptCoreAdminObject() {
  getScriptObject()->vcpu_setClassName("CoreAdmin");
  getScriptObject()->vcpu_setController(coreAdminController);
  getScriptObject()->vcpu_setInterface(COREADMIN_SCRIPTOBJECT_GUID, static_cast<ScriptCoreAdminObject *>(this));

#ifndef FAKE_SCRIPTCORE
  waServiceFactory *s = api->service_enumService(WaSvc::COREADMIN,0);
  ASSERTPR(s,"Core Admin must be present to use ScriptCore!");
  admin = castService<svc_coreAdmin>(s);
#endif
}

ScriptCoreAdminObject::~ScriptCoreAdminObject() {
#ifndef FAKE_SCRIPTCORE
  api->service_release(admin);
  admin = NULL;
#endif
}

CoreToken ScriptCoreAdminObject::getNamedCore(const char *name) {
#ifndef FAKE_SCRIPTCORE
  return admin->nameToToken(name);
#else
  return 0;
#endif  
}

CoreToken ScriptCoreAdminObject::newNamedCore(const char *name) {
#ifndef FAKE_SCRIPTCORE
  return admin->createCore(name);
#else
  return 0;
#endif
}

int ScriptCoreAdminObject::freeCore(CoreToken core) {
#ifndef FAKE_SCRIPTCORE
  if (core == 0) return 0;  // don't touch the main core.
  return admin->freeCoreByToken(core);
#else
  return 0;
#endif
}

int ScriptCoreAdminObject::freeCoreByName(const char *name) {
#ifndef FAKE_SCRIPTCORE
  CoreToken core = admin->nameToToken(name);
  if (core == 0) return 0;  // don't touch the main core.
  return admin->freeCoreByToken(core);
#else
  return 0;
#endif
}

