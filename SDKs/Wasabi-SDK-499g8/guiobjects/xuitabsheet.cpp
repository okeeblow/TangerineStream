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
#include "xuitabsheet.h"
#include "../bfc/paramparser.h"

static ScriptTabSheetController TabsheetController;
ScriptTabSheetController *tabsheetController=&TabsheetController;

// -----------------------------------------------------------------------
char ScriptTabSheetXuiObjectStr[] = "Wasabi:TabSheet"; // This is the xml tag
char ScriptTabSheetXuiSvcName[] = "Wasabi:TabSheet XuiObject Service"; 

// -----------------------------------------------------------------------
ScriptTabSheet::ScriptTabSheet() : TypeSheet(0) {
  getScriptObject()->vcpu_setInterface(tabsheetGuid, (void *)static_cast<ScriptTabSheet*>(this));
  getScriptObject()->vcpu_setClassName("TabSheet"); // this is the script class name
  getScriptObject()->vcpu_setController(tabsheetController);

  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "windowtype", SCRIPTTABSHEET_SETWINDOWTYPE, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "children", SCRIPTTABSHEET_SETCHILDREN, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "type", SCRIPTTABSHEET_SETTYPE, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "content_margin_left", SCRIPTTABSHEET_SETCONTENTMARGINLEFT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "content_margin_top", SCRIPTTABSHEET_SETCONTENTMARGINTOP, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "content_margin_right", SCRIPTTABSHEET_SETCONTENTMARGINRIGHT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "content_margin_bottom", SCRIPTTABSHEET_SETCONTENTMARGINBOTTOM, XUI_ATTRIBUTE_IMPLIED);
  wndtype = 0;
  type = TABSHEET_GROUPS;
  left_margin = right_margin = bottom_margin = top_margin = 0;
}

// -----------------------------------------------------------------------
ScriptTabSheet::~ScriptTabSheet() {
  children_id.deleteAll();
}

// -----------------------------------------------------------------------
int ScriptTabSheet::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTTABSHEET_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case  SCRIPTTABSHEET_SETWINDOWTYPE:
      setWindowType(value);
      break;
    case  SCRIPTTABSHEET_SETCHILDREN:
      setChildrenIds(value);
      break;
    case  SCRIPTTABSHEET_SETTYPE:
      setType(value);
      break;
    case  SCRIPTTABSHEET_SETCONTENTMARGINLEFT:
    case  SCRIPTTABSHEET_SETCONTENTMARGINTOP:
    case  SCRIPTTABSHEET_SETCONTENTMARGINRIGHT:
    case  SCRIPTTABSHEET_SETCONTENTMARGINBOTTOM:
      setContentMarginX(value, xmlattributeid);
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void ScriptTabSheet::setWindowType(const char *paramvalue) {
  if (STREQLSAFE(wndtype, paramvalue)) return;
  wndtype = paramvalue;
  reloadChildren();
}

// -----------------------------------------------------------------------
void ScriptTabSheet::setChildrenIds(const char *paramvalue) {
  if (STREQLSAFE(paramvalue, "")) {
    children_id.removeAll();
  }
  ParamParser pp(paramvalue);
  for (int i=0;i<pp.getNumItems();i++) 
    children_id.addItem(new String(pp.enumItem(i)));
  reloadChildren();
}

// -----------------------------------------------------------------------
void ScriptTabSheet::setType(const char *paramvalue) {
  int ttype = ATOI(paramvalue);
  if (type == ttype) return;
  type = ttype;
  setButtonType(type);
}

// -----------------------------------------------------------------------
void ScriptTabSheet::reloadChildren() {
  if (!isInited()) return;
  killChildren();
  for (int i=0;i<children_id.getNumItems();i++) {
    GuiObjectWnd *w = new GuiObjectWnd;
    //w->abstract_setAllowDeferredContent(1);
    w->setContent(children_id.enumItem(i)->getValue());
    addChild(w);
  }
  TypeSheet::setWindowType(wndtype);
  if (!wndtype.isempty()) 
    TypeSheet::load();
}

// -----------------------------------------------------------------------
int ScriptTabSheet::onInit() {
  int r = SCRIPTTABSHEET_PARENT::onInit();
  setButtonType(type);
  reloadChildren();
  return r;
}

void ScriptTabSheet::setContentMarginX(const char *value, int what) {
  switch (what) {
    case SCRIPTTABSHEET_SETCONTENTMARGINLEFT:
      setContentMarginLeft(ATOI(value));
      break;
    case SCRIPTTABSHEET_SETCONTENTMARGINTOP:
      setContentMarginTop(ATOI(value));
      break;
    case SCRIPTTABSHEET_SETCONTENTMARGINRIGHT:
      setContentMarginRight(ATOI(value));
      break;
    case SCRIPTTABSHEET_SETCONTENTMARGINBOTTOM:
      setContentMarginBottom(ATOI(value));
      break;
  }
}

// -----------------------------------------------------------------------
// Script Object

// -- Functions table -------------------------------------
function_descriptor_struct ScriptTabSheetController::exportedFunction[] = {
  {"getCurPage",             0, (void*)ScriptTabSheetController::tabsheet_getCurPage },
  {"setCurPage",             1, (void*)ScriptTabSheetController::tabsheet_setCurPage },
};

ScriptObject *ScriptTabSheetController::instantiate() {
  ScriptTabSheet *sts = new ScriptTabSheet;
  ASSERT(sts != NULL);
  return sts->getScriptObject();
}

void ScriptTabSheetController::destroy(ScriptObject *o) {
  ScriptTabSheet *sts = static_cast<ScriptTabSheet *>(o->vcpu_getInterface(tabsheetGuid));
  ASSERT(sts != NULL);
  delete sts;
}

void *ScriptTabSheetController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for tabsheet yet
}

void ScriptTabSheetController::deencapsulate(void *o) {
}

int ScriptTabSheetController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *ScriptTabSheetController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar ScriptTabSheetController::tabsheet_getCurPage(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTabSheet *sts = static_cast<ScriptTabSheet *>(o->vcpu_getInterface(tabsheetGuid));
  int a = 0;
  if (sts) a = sts->getCurPage();
  return MAKE_SCRIPT_INT(a);
}

scriptVar ScriptTabSheetController::tabsheet_getNumPages(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTabSheet *sts = static_cast<ScriptTabSheet *>(o->vcpu_getInterface(tabsheetGuid));
  int a = 0;
  if (sts) a = sts->getNumPages();
  return MAKE_SCRIPT_INT(a);
}

scriptVar ScriptTabSheetController::tabsheet_setCurPage(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a) {
  SCRIPT_FUNCTION_INIT
  ScriptTabSheet *sts = static_cast<ScriptTabSheet *>(o->vcpu_getInterface(tabsheetGuid));
  int _a = GET_SCRIPT_INT(a);
  if (sts) sts->setCurPage(_a);
  RETURN_SCRIPT_VOID;
}

scriptVar ScriptTabSheetController::tabsheet_nextPage(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTabSheet *sts = static_cast<ScriptTabSheet *>(o->vcpu_getInterface(tabsheetGuid));
  if (sts) sts->nextPage();
  RETURN_SCRIPT_VOID;
}

scriptVar ScriptTabSheetController::tabsheet_previousPage(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTabSheet *sts = static_cast<ScriptTabSheet *>(o->vcpu_getInterface(tabsheetGuid));
  if (sts) sts->previousPage();
  RETURN_SCRIPT_VOID;
}
