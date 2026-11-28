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
#include "xuiquerylist.h"
#include "../bfc/canvas.h"

#include "../studio/services/svc_objectdir.h"

// -----------------------------------------------------------------------
char QueryListXuiObjectStr[] = "QueryResults"; // This is the xml tag
char QueryListXuiSvcName[] = "ScriptQueryResults XuiObject Service";

// -----------------------------------------------------------------------
ScriptQueryList::ScriptQueryList() {
  getScriptObject()->vcpu_setInterface(queryListGuid, (void *)static_cast<ScriptQueryList *>(this));
#ifdef WASABI_COMPILE_METADB
  getScriptObject()->vcpu_setInterface(multiQueryServerGuid, (void *)static_cast<MultiQueryServer *>(this));
#endif
  getScriptObject()->vcpu_setClassName("QueryList"); // this is the script class name
  getScriptObject()->vcpu_setController(queryListController);

  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "title", QUERYLIST_SETTITLE, XUI_ATTRIBUTE_IMPLIED);
}

ScriptQueryList::~ScriptQueryList() {
}

// -----------------------------------------------------------------------
int ScriptQueryList::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return QUERYLIST_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case QUERYLIST_SETTITLE: setTitle(value);  break;
    default: return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void ScriptQueryList::setTitle(const char *name) {
  setName(name);
//CUT  if (!name || !*name) showLabel(0); else showLabel(1);
}

#ifdef WASABI_COMPILE_METADB
// -----------------------------------------------------------------------
void ScriptQueryList::onResetSubqueries() {
  QUERYLIST_PARENT::onResetSubqueries();
  QueryListScriptController::querylist_onResetQuery(SCRIPT_CALL, getScriptObject());
}
#endif

#ifdef WASABI_COMPILE_METADB
int ScriptQueryList::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source) {
  if (STRCASEEQLSAFE(action, "SAVEQUERY")) {
    String q = mqs_getMultiQuery();
    if (!q.isempty()) {
      svc_objectDir *qd = ObjectDirEnum("querydir").getNext();
      if (qd != NULL) {
        qd->insertObject(q, "gay", "user is gay");
      }
    }
    return 1;
  }
  return QUERYLIST_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, source);
}
#endif

// -----------------------------------------------------------------------
// Script Object

QueryListScriptController _queryListController;
QueryListScriptController *queryListController = &_queryListController;

// -- Functions table -------------------------------------
function_descriptor_struct QueryListScriptController::exportedFunction[] = {
  {"onResetQuery",            0, (void*)QueryListScriptController::querylist_onResetQuery},
};
                                      
ScriptObject *QueryListScriptController::instantiate() {
  ScriptQueryList *sql = new ScriptQueryList;
  ASSERT(sql != NULL);
  return sql->getScriptObject();
}

void QueryListScriptController::destroy(ScriptObject *o) {
  ScriptQueryList *sql = static_cast<ScriptQueryList *>(o->vcpu_getInterface(queryListGuid));
  ASSERT(sql != NULL);
  delete sql;
}

void *QueryListScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for querylist yet
}

void QueryListScriptController::deencapsulate(void *o) {
}

int QueryListScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *QueryListScriptController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar QueryListScriptController::querylist_onResetQuery(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT; 
  PROCESS_HOOKS0(o, queryListController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}
