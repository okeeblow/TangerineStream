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
#include "xuifilterlist.h"
#include "xuiquerylist.h"
#include "../bfc/canvas.h"
#include "../common/script/c_script/c_group.h"

#define CB_SETQUERYLIST 0x93
#define CB_INITQUERY     0x6892

// -----------------------------------------------------------------------
char FilterListXuiObjectStr[] = "FilterList"; // This is the xml tag
char FilterListXuiSvcName[] = "ScriptFilterList XuiObject Service";

// -----------------------------------------------------------------------
ScriptFilterList::ScriptFilterList() {
  getScriptObject()->vcpu_setInterface(filterListGuid, (void *)static_cast<ScriptFilterList *>(this));
  getScriptObject()->vcpu_setClassName("FilterList"); // this is the script class name
  getScriptObject()->vcpu_setController(filterListController);

  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "field", FILTERLIST_SETFIELD, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "querylist", FILTERLIST_SETQUERYLIST, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "title", FILTERLIST_SETTITLE, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "all", FILTERLIST_SETALLSTRING, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "order", FILTERLIST_SETORDER, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "linked", FILTERLIST_SETLINKED, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "querydirection", FILTERLIST_SETQUERYDIRECTION, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "forcevirtual", FILTERLIST_FORCEVIRTUAL, XUI_ATTRIBUTE_IMPLIED);


  allstr = "All";
  serv = NULL;
}

ScriptFilterList::~ScriptFilterList() {
}

// -----------------------------------------------------------------------
int ScriptFilterList::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return FILTERLIST_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case FILTERLIST_SETFIELD:
      setMetadataField(value);
      break;
    case FILTERLIST_SETQUERYLIST:
      setXuiQueryList(value);
      break;
    case FILTERLIST_SETTITLE:
      setTitle(value);
      break;
    case FILTERLIST_SETALLSTRING:
      setAllString(value);
      break;
    case FILTERLIST_SETORDER:
      setOrder(ATOI(value));
      break;
    case FILTERLIST_SETLINKED:
      setLinked(!!ATOI(value));
      break;
    case FILTERLIST_SETQUERYDIRECTION:
      setQueryDirection(ATOI(value));
      break;
    case FILTERLIST_FORCEVIRTUAL:
      setVirtual(ATOI(value));
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void ScriptFilterList::setTitle(const char *name) {
  setName(name);
  if (!name || !*name) showLabel(0); else showLabel(1);
}

// -----------------------------------------------------------------------
void ScriptFilterList::setAllString(const char *name) {
  allstr = name;
}

const char *ScriptFilterList::getAllString() {
  return allstr;
}

// -----------------------------------------------------------------------
int ScriptFilterList::onInit() {
  FILTERLIST_PARENT::onInit();
  if (!querylist_id.isempty())
    postDeferredCallback(CB_SETQUERYLIST, 0);
  return 1;
}

// -----------------------------------------------------------------------
void ScriptFilterList::setXuiQueryList(const char *v) {
  if (!isInited()) {
    querylist_id = v;
    return;
  }

  ScriptObject *o = WASABI_API_MAKI->maki_findObject(v);
  if (!o) return;
  ScriptQueryList *querylist = static_cast<ScriptQueryList *>(o->vcpu_getInterface(queryListGuid));
  if (!querylist) return;
  
  sqs_setMultiQueryServer(querylist);
}

// -----------------------------------------------------------------------
int ScriptFilterList::onDeferredCallback(int p1, int p2) {
  switch (p1) {
    case CB_SETQUERYLIST:
      setXuiQueryList(querylist_id);
      postDeferredCallback(CB_INITQUERY, 0);
      break;
    case CB_INITQUERY:
      sqs_setQuery(NULL);
      break;
    default:
      return FILTERLIST_PARENT::onDeferredCallback(p1, p2);

  }
  return 1;
}

// -----------------------------------------------------------------------
// Script Object

FilterListScriptController _filterListController;
FilterListScriptController *filterListController = &_filterListController;

// -- Functions table -------------------------------------
function_descriptor_struct FilterListScriptController::exportedFunction[] = {
  {"setQueryList",            1, (void*)FilterListScriptController::Filterlist_setQueryList },
};
                                      
ScriptObject *FilterListScriptController::instantiate() {
  ScriptFilterList *sql = new ScriptFilterList;
  ASSERT(sql != NULL);
  return sql->getScriptObject();
}

void FilterListScriptController::destroy(ScriptObject *o) {
  ScriptFilterList *sql = static_cast<ScriptFilterList *>(o->vcpu_getInterface(filterListGuid));
  ASSERT(sql != NULL);
  delete sql;
}

void *FilterListScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for Filterlist yet
}

void FilterListScriptController::deencapsulate(void *o) {
}

int FilterListScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *FilterListScriptController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar FilterListScriptController::Filterlist_setQueryList(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar id) {
  SCRIPT_FUNCTION_INIT
  ScriptFilterList *sql = static_cast<ScriptFilterList*>(o->vcpu_getInterface(filterListGuid));
  if (sql) sql->setXuiQueryList(GET_SCRIPT_STRING(id));
  RETURN_SCRIPT_VOID;  
}
