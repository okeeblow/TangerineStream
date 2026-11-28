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
#ifndef __SCRIPTOBJI_H
#define __SCRIPTOBJI_H

#include "vcputypes.h"
#include "../../bfc/dispatch.h"

#include "../../bfc/tlist.h"
#include "../../bfc/ptrlist.h"
#include "../../bfc/pair.h"

//<?<autoheader/>
#include "scriptobj.h"
#include "scriptobjx.h"

class ScriptHook;
class ScriptObject;
class ScriptObjectController;

//?>

using namespace wasabi;

typedef struct {
  int scriptid;
  int varid;
  TList<int> dlfs;
} assvar;

class ScriptObjectController;
class ScriptHook;

/*[interface.header.h]
enum {
  INTERFACE_GENERICVOIDPTR=0,
  INTERFACE_SCRIPTOBJECT,
};
*/

// ----------------------------------------------------------------------------------------------------------
class MemberVar {
  public:
    MemberVar(const char *name, int scriptid, int rettype);
    virtual ~MemberVar();

    const char *getName() { return name; }
    int getScriptId() { return scriptid; }
    int getReturnType() { return rettype; }
    int getGlobalId() { return globalid; }

  private:  
    String name;
    int scriptid;
    int rettype;
    int globalid;
};

/*class MemberVarCompare {
  public:
    static int compareItem(void *p1, void *p2);
    static int compareAttrib(const char *attrib, void *p);
};*/

// ----------------------------------------------------------------------------------------------------------

class InterfaceEntry {
  public:
    InterfaceEntry(GUID _guid, void *_ptr, int _type = INTERFACE_GENERICVOIDPTR) : guid(_guid), type(_type), ptr(_ptr) {}
    virtual ~InterfaceEntry() {}

    virtual GUID getGuid() { return guid; }
    virtual void *getInterface() { return ptr; }
    virtual int getType() { return type; }

  private:

    GUID guid;
    void *ptr;
    int type;
};


// ----------------------------------------------------------------------------------------------------------

class COMEXP ScriptObjectI : public ScriptObjectX {

public:
  ScriptObjectI(const char *class_name = NULL, ScriptObjectController *object_controller = NULL);
  virtual ~ScriptObjectI();

  DISPATCH(50) virtual void *vcpu_getInterface(GUID g, int *interfacetype = NULL);
  DISPATCH(60) virtual void *vcpu_getInterfaceObject(GUID g, ScriptObject **o);
  DISPATCH(100) int vcpu_getAssignedVariable(int start, int scriptid, int functionId, int *next, int *globalevententry, int *inheritedevent);
  DISPATCH(200) void vcpu_removeAssignedVariable(int var, int id);
  DISPATCH(300) void vcpu_addAssignedVariable(int var, int scriptid);
  DISPATCH(400) virtual const char *vcpu_getClassName();
  DISPATCH(500) virtual ScriptObjectController *vcpu_getController();
//  DISPATCH(600) virtual void vcpu_addClassHook(ScriptHook *h);
//  DISPATCH(700) virtual void vcpu_addObjectHook(ScriptHook *h);
  DISPATCH(800) int vcpu_getScriptId();
  DISPATCH(900) void vcpu_setScriptId(int i);
  DISPATCH(1000) int vcpu_getMember(const char *id, int scriptid, int rettype);
  DISPATCH(1100) void vcpu_delMembers(int scriptid);
  DISPATCH(1200) virtual void vcpu_setInterface(GUID g, void *v, int interfacetype = INTERFACE_SCRIPTOBJECT);
  DISPATCH(1300) virtual void vcpu_setClassName(const char *name);
  DISPATCH(1400) virtual void vcpu_setController(ScriptObjectController *c);
  DISPATCH(1500) virtual void vcpu_init();

protected:
  NODISPATCH int getEventForVar(assvar *var, int funcid, int *inheritedevent);
  NODISPATCH void computeEventList(assvar *a);
  PtrList < assvar > assignedVariables;
  PtrList < MemberVar > memberVariables;
  int cache_count;
  int id;
  String membercacheid;
  int membercachesid;
  int membercachegid;
  PtrList < InterfaceEntry > interfaceslist;
  String classname;
  ScriptObjectController * controller;
  int ingetinterface;

};

#endif
