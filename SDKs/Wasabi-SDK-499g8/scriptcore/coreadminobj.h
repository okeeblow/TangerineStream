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
#ifndef __COREADMINOBJ_H
#define __COREADMINOBJ_H

#include <common/script/objcontroller.h>
#include <common/script/rootobj.h>
#include <core/sequence.h>
#include <core/corecb.h>

#ifdef GEN_FF
#ifndef FAKE_SCRIPTCORE
#define FAKE_SCRIPTCORE
#endif
#endif

class svc_coreAdmin;

// {F857BECA-8E19-41f1-973E-097E39649F03}
static const GUID COREADMIN_SCRIPTOBJECT_GUID = 
{ 0xf857beca, 0x8e19, 0x41f1, { 0x97, 0x3e, 0x9, 0x7e, 0x39, 0x64, 0x9f, 0x3 } };

extern ScriptObjectController *coreAdminController;

// -----------------------------------------------------------------------------------------------------
class ScriptCoreAdminObject : public RootObjectInstance {
public:
  ScriptCoreAdminObject();
  virtual ~ScriptCoreAdminObject();

  CoreToken getNamedCore(const char *name);
  CoreToken newNamedCore(const char *name);
  int freeCore(CoreToken core);
  int freeCoreByName(const char *name);

private:
#ifndef FAKE_SCRIPTCORE
  svc_coreAdmin *admin;
#endif
};

// -----------------------------------------------------------------------------------------------------
class CoreAdminScriptObjectController : public ScriptObjectControllerI {
public:
  virtual const char *getClassName() { return "CoreAdmin"; } 
  virtual const char *getAncestorClassName() { return "Object"; }  
  virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(rootObjectGuid); } 
  virtual int getNumFunctions(); 
  virtual const function_descriptor_struct *getExportedFunctions() { return exportedFunction; } 
  virtual GUID getClassGuid() { return COREADMIN_SCRIPTOBJECT_GUID; } 
  virtual int getInstantiable() { return 0; } 
  virtual int getReferenceable() { return 0; } 
  virtual ScriptObject *instantiate(); 
  virtual void *encapsulate(ScriptObject *o); 
  virtual void destroy(ScriptObject *o); 
  virtual void deencapsulate(void *o); 

  static scriptVar coreadmin_getNamedCore(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar name);
  static scriptVar coreadmin_newNamedCore(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar name);
  static scriptVar coreadmin_freeCore(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar core);
  static scriptVar coreadmin_freeCoreByName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar name);

private:
  static function_descriptor_struct exportedFunction[];
};

#endif
