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
#include "../../studio/api.h"
#include "scriptobji.h"
#include "objcontroller.h"
#include "scriptguid.h"

ScriptObjectI::ScriptObjectI(const char *class_name, ScriptObjectController *object_controller) {
  classname = class_name;
  controller = object_controller;
  cache_count = -1;
  membercachegid = -1;
  membercachesid = -1;
  ingetinterface = 0;
  vcpu_init();
}

ScriptObjectI::~ScriptObjectI() {
  assignedVariables.deleteAll();
  memberVariables.deleteAll();
  interfaceslist.deleteAll();
  WASABI_API_MAKI->vcpu_removeScriptObject(this);
}

void *ScriptObjectI::vcpu_getInterface(GUID g, int *interfacetype) {
  if (g == scriptObjectGuid) return this;
  foreach(interfaceslist)
    if (interfaceslist.getfor()->getGuid() == g) {
      if (interfacetype != NULL)
        *interfacetype = interfaceslist.getfor()->getType();
      return interfaceslist.getfor()->getInterface(); 
    }
  endfor;
  if (ingetinterface) return NULL;
  ingetinterface = 1;

  void *i = NULL;
  ScriptObjectController *c = controller;
  ScriptObject *no = NULL;
  while (i == NULL && c != NULL) {
    i = c->cast(this, g);
    if (i != NULL) break;
    c = c->getAncestorController();
  }

  if (interfacetype != NULL)
    *interfacetype = INTERFACE_SCRIPTOBJECT;
  ingetinterface = 0;
  return i;
}

void *ScriptObjectI::vcpu_getInterfaceObject(GUID g, ScriptObject **o) {
  if (g == scriptObjectGuid) return this;
  foreach(interfaceslist)
    if (interfaceslist.getfor()->getGuid() == g) {
      *o = NULL;
      return interfaceslist.getfor()->getInterface(); 
    }
  endfor;
  if (ingetinterface) return NULL;
  ingetinterface = 1;

  void *i = NULL;
  ScriptObjectController *c = controller;
  ScriptObject *no = NULL;
  while (i == NULL && c != NULL) {
    i = c->cast(this, g);
    if (i != NULL) {
      if (o != NULL)
      *o = (ScriptObject *)i;
      break;
    }
    c = c->getAncestorController();
  }

  ingetinterface = 0;
  return i;
}

int ScriptObjectI::vcpu_getAssignedVariable(int start, int scriptid, int functionId, int *next, int *globalevententry, int *inheritedevent) {
  if (start < 0) start = 0;
  if (start >= assignedVariables.getNumItems()) return -1;
  for (int i=start;i<assignedVariables.getNumItems();i++) {
    assvar *v = assignedVariables.enumItem(i);
    if (WASABI_API_MAKI->vcpu_getCacheCount() != cache_count) {
      if (!WASABI_API_MAKI->vcpu_isValidScriptId(v->scriptid)) {
        vcpu_removeAssignedVariable(v->varid, v->scriptid);
        i--;
        continue;
      }
    }
    if (scriptid == -1 || v->scriptid == scriptid) {
      int r = getEventForVar(v, functionId, inheritedevent);
      if (r == -1) continue;
      if (next) *next = i+1;
      if (globalevententry) *globalevententry = r;
      return WASABI_API_MAKI->vcpu_mapVarId(v->varid, v->scriptid);
    }    
  }
  return -1;
}

void ScriptObjectI::vcpu_removeAssignedVariable(int var, int id) {
  for (int i=0;i<assignedVariables.getNumItems();i++) {
    assvar *v = assignedVariables.enumItem(i);
    if (v->varid == var && v->scriptid == id) {
      delete v;
      assignedVariables.removeItem(v);
      return;
    }
  }
}

void ScriptObjectI::vcpu_addAssignedVariable(int var, int scriptid) {
  do {
    assvar *v = new assvar;
    v->scriptid = scriptid;
    v->varid = var;
    assignedVariables.addItem(v);
    computeEventList(v);
    var = WASABI_API_MAKI->vcpu_getUserAncestorId(var, scriptid);
  } while (var != -1);
}

const char *ScriptObjectI::vcpu_getClassName() {
  return classname;
}

ScriptObjectController *ScriptObjectI::vcpu_getController() {
  return controller;
}

int ScriptObjectI::vcpu_getScriptId() {
  return id;
}

void ScriptObjectI::vcpu_setScriptId(int i) {
  id = i;
}

int ScriptObjectI::vcpu_getMember(const char *id, int scriptid, int rettype) {
  if (membercachesid == scriptid && STRCASEEQL(membercacheid, id)) 
    return membercachegid;
  membercacheid = id;
  membercachesid = scriptid;
  for (int i=0;i<memberVariables.getNumItems();i++) {
    MemberVar *m = memberVariables.enumItem(i);
    if (m->getScriptId() == scriptid && STRCASEEQL(m->getName(), id)) {
      membercachegid = m->getGlobalId();
      return membercachegid;
    }
  }
  MemberVar *m = new MemberVar(id, scriptid, rettype);
  memberVariables.addItem(m);
  membercachegid = m->getGlobalId();
  return membercachegid;
}

void ScriptObjectI::vcpu_delMembers(int scriptid) {
  for (int i=0;i<memberVariables.getNumItems();i++)
    if (memberVariables.enumItem(i)->getScriptId() == scriptid) {
      delete memberVariables.enumItem(i);
      memberVariables.removeByPos(i--);
    }
}

void ScriptObjectI::vcpu_setInterface(GUID g, void *v, int interfacetype) {
  for (int i=0;i<interfaceslist.getNumItems();i++)
    if (interfaceslist.enumItem(i)->getGuid() == g) {
      InterfaceEntry *p = interfaceslist.enumItem(i);
      delete p;
      interfaceslist.removeByPos(i);
      i--;
    }
  interfaceslist.addItem(new InterfaceEntry(g,v,interfacetype));
}

void ScriptObjectI::vcpu_setClassName(const char *name) {
  classname = name;
}

void ScriptObjectI::vcpu_setController(ScriptObjectController *c) {
  controller = c;
}

void ScriptObjectI::vcpu_init() {
  WASABI_API_MAKI->vcpu_addScriptObject(this);
}

int ScriptObjectI::getEventForVar(assvar *var, int funcid, int *inheritedevent) {
  if (WASABI_API_MAKI->vcpu_getCacheCount() != cache_count) {
    for (int i=0;i<assignedVariables.getNumItems();i++)
      computeEventList(assignedVariables.enumItem(i));
    cache_count = WASABI_API_MAKI->vcpu_getCacheCount();
  }
  TList<int> *list = &var->dlfs;
  for (int i=0;i<list->getNumItems();i+=4) 
    if (list->enumItem(i) == funcid && list->enumItem(i+1) == var->varid) {
      *inheritedevent = list->enumItem(i+3);
      return list->enumItem(i+2);
    }

  return -1;
}

void ScriptObjectI::computeEventList(assvar *a) {
  a->dlfs.removeAll();

  int dlfid;
  int scriptid;
  int varid;

  int var = a->varid;
  int inheritedevent=0;

  do {
    for (int i=0;i<WASABI_API_MAKI->vcpu_getNumEvents();i++) {
      WASABI_API_MAKI->vcpu_getEvent(i, &dlfid, &scriptid, &varid);
      if (scriptid == a->scriptid && varid == var) {
        a->dlfs.addItem(dlfid);
        a->dlfs.addItem(varid);
        a->dlfs.addItem(i);
        a->dlfs.addItem(inheritedevent);
      }
    }
    var = WASABI_API_MAKI->vcpu_getUserAncestorId(var, a->scriptid);
    inheritedevent = 1;
  } while (var != -1);
}



MemberVar::MemberVar(const char *_name, int _scriptid, int _rettype) {
  name = _name;
  rettype = _rettype;
  scriptid = _scriptid;
  globalid = WASABI_API_MAKI->maki_createOrphan(rettype);
}

MemberVar::~MemberVar() {
  WASABI_API_MAKI->maki_killOrphan(globalid); // heh :)
}

