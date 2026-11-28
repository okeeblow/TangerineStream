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
#ifndef __CFGSCRIPTOBJ_H
#define __CFGSCRIPTOBJ_H

#include "../common/script/rootobj.h"
#include "../common/script/objcontroller.h"
#include "../common/script/scriptobj.h"
#include "../bfc/depend.h"
#include "../studio/services/svc_scriptobj.h"

class PlDirObject;

extern ScriptObjectController *pldirController;

// -----------------------------------------------------------------------------------------------------
// ScriptObject Service
class PlDirScriptObjectSvc : public svc_scriptObjectI {

public:
  PlDirScriptObjectSvc() {};
  virtual ~PlDirScriptObjectSvc() {};

  static const char *getServiceName() { return "PlDir Script Object Service"; }
  virtual ScriptObjectController *getController(int n);
};       

// -----------------------------------------------------------------------------------------------------
// Script classe GUIDS

// {61A7ABAD-7D79-41f6-B1D0-E1808603A4F4}
static const GUID PLDIR_SCRIPTOBJECT_GUID = 
{ 0x61a7abad, 0x7d79, 0x41f6, { 0xb1, 0xd0, 0xe1, 0x80, 0x86, 0x3, 0xa4, 0xf4 } };

// -----------------------------------------------------------------------------------------------------
// ScriptObject Interface

//   PlDir 
class PlDirObject : public RootObjectInstance {
  
  public:

    PlDirObject();
    virtual ~PlDirObject();

    // virtual void doSomething(); // for non static script class
    static void showCurrentlyPlayingEntry();
    
  private:

};

// -----------------------------------------------------------------------------------------------------
// ScriptObjectControllers for our script classes

//   PlDir
class PlDirScriptObjectController : public ScriptObjectControllerI {
  public:
    virtual const char *getClassName() { return "PlDir"; }
    virtual const char *getAncestorClassName() { return "Object"; }
    virtual ScriptObjectController *getAncestorController() { return NULL; }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions() { return exportedFunction; }
    virtual GUID getClassGuid() { return PLDIR_SCRIPTOBJECT_GUID; }
    virtual int getInstantiable() { return 0; }
    virtual int getReferenceable() { return 0; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

    static scriptVar pldir_showCurrentlyPlayingEntry(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

  private:
    static function_descriptor_struct exportedFunction[];
};


#endif

