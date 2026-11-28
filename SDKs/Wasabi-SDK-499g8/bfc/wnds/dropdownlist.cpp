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
#include "dropdownlist.h"
#include "listwnd.h"
#include "../../common/script/guiobject.h"
#include "../../common/script/scriptguid.h"
#include "../../common/script/c_script/c_text.h"
#include "../../studio/api.h"
#include "../popexitcb.h"
#include "../notifmsg.h"
#include "../svc_enum.h"
#include "../paramparser.h"
#include "../../studio/services/svc_textfeed.h"

#define DDL_CLOSELISTCB 0x0721

// -----------------------------------------------------------------------
DropDownList::DropDownList() {
  selected = -1;
  //abstract_setAllowDeferredContent(1);
  clicks_button = NULL;
  clicks_text = NULL;
  list_key = NULL;
  height = 128;
  maxitems = 0;
  noitemtext = "";
  list_group = NULL;
  trap_click = 0;
  disable_cfg_event = 0;

  GuiObjectWnd::getScriptObject()->vcpu_setInterface(dropDownListGuid, (void *)static_cast<DropDownList *>(this));
  GuiObjectWnd::getScriptObject()->vcpu_setClassName("DropDownList"); // this is the script class name
  GuiObjectWnd::getScriptObject()->vcpu_setController(dropDownListController);

  myxuihandle = newXuiHandle();
                         
  addParam(myxuihandle, "items", DROPDOWNLIST_SETITEMS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "feed", DROPDOWNLIST_SETFEED, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "select", DROPDOWNLIST_SELECT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "listheight", DROPDOWNLIST_LISTHEIGHT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "maxitems", DROPDOWNLIST_MAXITEMS, XUI_ATTRIBUTE_IMPLIED);

  registerAcceleratorSection("popup", 1);
}

// -----------------------------------------------------------------------
DropDownList::~DropDownList() { 
  doCloseList(0);
  delete clicks_text;
  delete clicks_button;
  delete list_key;
}

// -----------------------------------------------------------------------
int DropDownList::onAcceleratorEvent(const char *name) {
  int r = DROPDOWNLIST_PARENT::onAcceleratorEvent(name);
  if (STRCASEEQLSAFE(name, "exit")) {
    escapeCallback();
    return 1;
  }
  return r;
}

// -----------------------------------------------------------------------
int DropDownList::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return DROPDOWNLIST_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case DROPDOWNLIST_SETITEMS:
      setItems(value);
      break;
    case DROPDOWNLIST_SETFEED:
      setFeed(value);
      break;
    case DROPDOWNLIST_SELECT:
      selectItem(findItem(value));
      break;
    case DROPDOWNLIST_LISTHEIGHT:
      setListHeight(ATOI(value));
      break;
    case DROPDOWNLIST_MAXITEMS:
      setMaxItems(ATOI(value));
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
int DropDownList::onInit() {
  int rt = DROPDOWNLIST_PARENT::onInit();
  abstract_setContent(dropdownlist_getMainGroupId());
  return rt;
}

// -----------------------------------------------------------------------
void DropDownList::abstract_onNewContent() {
  DROPDOWNLIST_PARENT::abstract_onNewContent();
  trapControls();
  updateTextInControl(getSelectedText());
}  

#ifdef WASABI_COMPILE_CONFIG
// -----------------------------------------------------------------------
int DropDownList::onReloadConfig() {
  int r = DROPDOWNLIST_PARENT::onReloadConfig();
  disable_cfg_event = 1;
  updateTextFromConfig(); // triggers onSelect
  disable_cfg_event = 0;
  return r;
}

// -----------------------------------------------------------------------
void DropDownList::updateTextFromConfig() { 
  const char *val = getGuiObject()->guiobject_getCfgString();
  const char *old = getSelectedText();
  if (old && val && STRCASEEQL(val, old)) return;

  if (val != NULL) {
    int id = findItem(val);
    if (id != -1)
      selectItem(id);
  }
}
#endif

// -----------------------------------------------------------------------
void DropDownList::trapControls() {
  delete clicks_button;
  delete clicks_text;

  clicks_button = NULL;
  clicks_text = NULL;

  if (wantTrapText()) {
    GuiObject *textGuiObj = getGuiObject()->guiobject_findObject(dropdownlist_getTextId());
    if (textGuiObj) clicks_text = new DDLClicksCallback(*textGuiObj, this);
  }

  if (wantTrapButton()) {
    GuiObject *butGuiObj = getGuiObject()->guiobject_findObject(dropdownlist_getButtonId());
    if (butGuiObj) clicks_button = new DDLClicksCallback(*butGuiObj, this);
  }
}


// -----------------------------------------------------------------------
void DropDownList::clickCallback() {
  if (list_group != NULL)
    closeList();
  else
    openList();
}

// -----------------------------------------------------------------------
void DropDownList::escapeCallback() {
  if (isListOpen())
    closeList();
}

// -----------------------------------------------------------------------
void DropDownList::openList() {
  onPreOpenList();
  WASABI_API_WND->appdeactivation_push_disallow(this);
#ifdef WASABI_COMPILE_WNDMGR
  list_group = WASABI_API_SKIN->group_create_layout(dropdownlist_getListGroupId());
#else
  list_group = WASABI_API_SKIN->group_create(dropdownlist_getListGroupId());
#endif
  group_dep = list_group->getDependencyPtr();
  viewer_addViewItem(group_dep);
  if (list_group == NULL) return;
  list_group->setStartHidden(1);
  list_group->setParent(WASABI_API_WND->main_getRootWnd());
  trap_click = 0;
  list_group->init(this, TRUE);
  setListParams();

  // At this point, the list should be good.  Calc for max-items size
  int calc_height = 0;
  if (maxitems) {
    RootWnd *listroot = list_group->findWindowByInterface(listGuid);
    ListWnd *listwnd = static_cast<ListWnd *>(listroot->getInterface(guilistGuid));
    GuiObject *listobj = listroot->getGuiObject();
    if (listwnd) {
      int numitems = 0;
      if (maxitems == -1) {
        numitems = listwnd->getNumItems(); 
      } else {
        numitems = min(maxitems, listwnd->getNumItems()); 
      }
      int offset_h = 0;
      if (listobj) {
         const char *y_param = listobj->guiobject_getXmlParam("y");
         const char *h_param = listobj->guiobject_getXmlParam("h");
         const char *ry_param = listobj->guiobject_getXmlParam("relaty");
         const char *rh_param = listobj->guiobject_getXmlParam("relath");
         int h_val = (h_param)?ATOI(h_param):0;
         int y_val = (y_param)?ATOI(y_param):0;
         if (ry_param && (strcmp(ry_param,"1") == 0)) {
           if (y_val < 0) y_val = -y_val;
           else y_val = 0;
         }
         if (rh_param && (strcmp(rh_param,"1") == 0)) {
           if (h_val < 0) h_val = -h_val;
         }
         offset_h = h_val + y_val;
      }
      calc_height = (numitems * listwnd->getItemHeight()) + offset_h;
    }
  } else {
    calc_height = height;
  }

  RECT r;
  getWindowRect(&r);
  int w = r.right-r.left;
  r.top = r.bottom;
  r.bottom = r.top + calc_height;
  divRatio(&r);
  list_group->resize(r.left, r.top, r.right-r.left, r.bottom-r.top);
  list_group->setVisible(1);
  WASABI_API_WND->popupexit_register(this, list_group); // this will call us back whenever someone clicks outside us
  trap_click = 1;
  listif = list_group->findWindowByInterface(listGuid);
  if (listif != NULL)
    list_key = new DDLKeyCallback(listif->getGuiObject()->guiobject_getScriptObject(), this);
  dropdownlist_onOpenList();
}

// -----------------------------------------------------------------------
void DropDownList::dropdownlist_onOpenList() {
  SetCapture(NULL); // NONPORTABLE, the goal is to cancel any capture some of our content guiobject might have so as to let the click down + slide in list transfer mouse capture
  setFocus();
}

// -----------------------------------------------------------------------
void DropDownList::dropdownlist_onCloseList() {
}

// -----------------------------------------------------------------------
void DropDownList::closeList() {
  if (list_group != NULL) {
    onPreCloseList();
    postDeferredCallback(DDL_CLOSELISTCB, 0);
  }
}

// -----------------------------------------------------------------------
void DropDownList::doCloseList(int cb) {
  if (cb) dropdownlist_onCloseList();
  if (list_group) {
    trap_click = 0;
    WASABI_API_WND->popupexit_deregister(this);
    WASABI_API_SKIN->group_destroy(list_group);
    list_group = NULL;
    group_dep = NULL;
    action_list = NULL;
    delete list_key;
    list_key = NULL;
    WASABI_API_WND->appdeactivation_pop_disallow(this);
  }
}

// -----------------------------------------------------------------------
void DropDownList::setListParams() {
  ASSERT(list_group != NULL);
  GuiObject *go = static_cast<GuiObject *>(list_group->getInterface(guiObjectGuid));
  if (go != NULL) {
    dropdownlist_onConfigureList(go);
  }
}

// -----------------------------------------------------------------------
void DropDownList::dropdownlist_onConfigureList(GuiObject *go) {
  XmlObject *o = NULL;
  if (go != NULL) {
    GuiObject *list = go->guiobject_findObject(dropdownlist_getListId());
    action_list = list->guiobject_getRootWnd();
    if (list != NULL) {
      o = static_cast<XmlObject *>(list->guiobject_getScriptObject()->vcpu_getInterface(xmlObjectGuid));
    }
  }
  String s;
  foreach(items) 
    if (foreach_index > 0) s += ";";
    s += items.getfor()->getText();
  endfor;
  o->setXmlParam("multiselect", "0");
  o->setXmlParam("hoverselect", "1");
  o->setXmlParam("selectonupdown", "0");
  o->setXmlParam("sort", StringPrintf("%d", wantAutoSort()));
  o->setXmlParam("items", s);
  if (selected != -1) o->setXmlParam("select", getSelectedText());
}

// -----------------------------------------------------------------------
int DropDownList::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source) {
  if (STRCASEEQLSAFE(action, "set_selection")) {
    int p = findItem(param);
    selectItem(p);
    return p;
  }
  if (STRCASEEQLSAFE(action, "get_selection")) {
    if (source)
      sendAction(source, "set_selection", getSelectedText());
  }
  return DROPDOWNLIST_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, source);
}

// -----------------------------------------------------------------------
int DropDownList::addItem(const char *text) {
  DDLEntry *e = new DDLEntry(text);
  items.setSorted(wantAutoSort());
  items.addItem(e);
  return e->getId();
}

// -----------------------------------------------------------------------
void DropDownList::selectDefault() {
#ifdef WASABI_COMPILE_CONFIG
  onReloadConfig();
#endif
}

// -----------------------------------------------------------------------
void DropDownList::delItem(int id) {
  foreach(items)
    if (items.getfor()->getId() == id) {
      delete items.getfor();
      items.removeByPos(foreach_index);
      break;
    }
  endfor;
  if (list_group != NULL)
    setListParams();
}

// -----------------------------------------------------------------------
void DropDownList::selectItem(int id, int hover) {
  //FG> DO NOT PUT THIS TEST BACK: if (selected == id) return;
  selected = id;
  onSelect(selected, hover);
}

// -----------------------------------------------------------------------
void DropDownList::onSelect(int id, int hover) {
  updateTextInControl(getSelectedText());
  if (!disable_cfg_event && !hover) {
#ifdef WASABI_COMPILE_CONFIG
    if (selected == -1)
      getGuiObject()->guiobject_setCfgString("");
    else
      getGuiObject()->guiobject_setCfgString(getSelectedText());
#endif
  }

  // Let the script have the callback, too.
  DropDownListScriptController::DropDownList_onSelect(SCRIPT_CALL, GuiObjectWnd::getScriptObject(), MAKE_SCRIPT_INT(id), MAKE_SCRIPT_INT(hover));

}

// -----------------------------------------------------------------------
const char *DropDownList::getItemText(int id) {
  foreach(items)
    if (items.getfor()->getId() == id)
      return items.getfor()->getText();
  endfor;
  return NULL;
}

// -----------------------------------------------------------------------
int DropDownList::findItem(const char *text) {
  int pos=-1;
  items.findItem(text, &pos);
  if (pos < 0) return -1;
  return items[pos]->getId();
}

// -----------------------------------------------------------------------
void DropDownList::updateTextInControl(const char *txt) {
  GuiObject *content = getContent();
  if (content != NULL) {
    if (wantTrapText()) {
      GuiObject *text = content->guiobject_findObject(dropdownlist_getTextId());
      if (text != NULL) {
        C_Text t(*text);
        t.setText(txt);
      }
    }
  }
}

// -----------------------------------------------------------------------
void DropDownList::setNoItemText(const char *txt) {
  noitemtext = txt;
  if (selected == -1)
    updateTextInControl(getSelectedText());
}

// -----------------------------------------------------------------------
int DropDownList::popupexitcb_onExitPopup() {
  closeList();
  return 1;
}

// -----------------------------------------------------------------------
int DropDownList::childNotify(RootWnd *child, int msg, int param1, int param2) {
  if (msg == ChildNotify::LISTWND_ITEMSELCHANGED && param2 && trap_click) {
    sendAction(action_list, "get_selection");
    closeList();
  }
  return DROPDOWNLIST_PARENT::childNotify(child, msg, param1, param2);
}

// -----------------------------------------------------------------------
int DropDownList::onDeferredCallback(int p1, int p2) {
  if (p1 == DDL_CLOSELISTCB)
    doCloseList();
  return DROPDOWNLIST_PARENT::onDeferredCallback(p1, p2);
}

// -----------------------------------------------------------------------
int DropDownList::viewer_onItemDeleted(Dependent *item) {
  if (item == group_dep) {
    WASABI_API_WND->popupexit_deregister(this);
    trap_click = 0;
    list_group = NULL;
    group_dep = NULL;
    action_list = NULL;
  }
  return 1;
}

// -----------------------------------------------------------------------

void DropDownList::feedwatcher_onSetFeed(svc_textFeed *svc) {
  String a = getRootWndName();
  if (a.isempty())
    setName(svc->getFeedDescription(getFeedId()));
}

void DropDownList::feedwatcher_onFeedChange(const char *data) {
  setItems(data);
}

// -----------------------------------------------------------------------
void DropDownList::deleteAllItems() {
  items.deleteAll();
  selected = -1;
}

// -----------------------------------------------------------------------
int DropDownList::onKeyDown(int keyCode) {
  if (isListOpen()) {
    switch (keyCode) {
      case VK_ESCAPE: 
        closeList();
        break;
    }
    if (listif != NULL) {
      listif->onKeyDown(keyCode);
    }
  } else {
    switch (keyCode) {
      case VK_SPACE: 
      case VK_RETURN: 
        openList();
        break;
    }
  }
  return DROPDOWNLIST_PARENT::onKeyDown(keyCode);
}

// -----------------------------------------------------------------------
int DropDownList::onKeyUp(int keyCode) {
  if (isListOpen()) {
    if (listif != NULL) {
      listif->onKeyUp(keyCode);
      return 1;
    }
  }
  return DROPDOWNLIST_PARENT::onKeyDown(keyCode);
}

// -----------------------------------------------------------------------
void DropDownList::setItems(const char *value) {
  deleteAllItems();
  ParamParser pp(value);
  for (int i=0;i<pp.getNumItems();i++) {
    addItem(pp.enumItem(i));
  }
  selectDefault();
}

// -----------------------------------------------------------------------

int DDLEntry::id_gen=0;


// -----------------------------------------------------------------------
// Script Object

DropDownListScriptController _dropDownListController;
DropDownListScriptController *dropDownListController = &_dropDownListController;

// -- Functions table -------------------------------------
function_descriptor_struct DropDownListScriptController::exportedFunction[] = {
  {"getItemSelected", 0, (void*)DropDownListScriptController::DropDownList_getItemSelected},
  {"onSelect",        2, (void*)DropDownListScriptController::DropDownList_onSelect},

  {"setListHeight",   1, (void*)DropDownListScriptController::DropDownList_setListHeight},
  {"openList",        0, (void*)DropDownListScriptController::DropDownList_openList},
  {"closeList",       0, (void*)DropDownListScriptController::DropDownList_closeList},
  {"setItems",        1, (void*)DropDownListScriptController::DropDownList_setItems},
  {"addItem",         1, (void*)DropDownListScriptController::DropDownList_addItem},
  {"delItem",         1, (void*)DropDownListScriptController::DropDownList_delItem},
  {"findItem",        1, (void*)DropDownListScriptController::DropDownList_findItem},
  {"getNumItems",     0, (void*)DropDownListScriptController::DropDownList_getNumItems},
  {"selectItem",      2, (void*)DropDownListScriptController::DropDownList_selectItem},
  {"getItemText",     1, (void*)DropDownListScriptController::DropDownList_getItemText},
  {"getSelected",     0, (void*)DropDownListScriptController::DropDownList_getSelected},
  {"getSelectedText", 0, (void*)DropDownListScriptController::DropDownList_getSelectedText},
  {"getCustomText",   0, (void*)DropDownListScriptController::DropDownList_getCustomText},
  {"deleteAllItems",  0, (void*)DropDownListScriptController::DropDownList_deleteAllItems},
  {"setNoItemText",   1, (void*)DropDownListScriptController::DropDownList_setNoItemText},
};
                                      
ScriptObject *DropDownListScriptController::instantiate() {
  DropDownList *ddl = new DropDownList;
  ASSERT(ddl != NULL);
  return ddl->GuiObjectWnd::getScriptObject();
}

void DropDownListScriptController::destroy(ScriptObject *o) {
  DropDownList *ddl= static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  ASSERT(ddl != NULL);
  delete ddl;
}

void *DropDownListScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for DropDownlist yet
}

void DropDownListScriptController::deencapsulate(void *o) {
}

int DropDownListScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *DropDownListScriptController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar DropDownListScriptController::DropDownList_getItemSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList*>(o->vcpu_getInterface(dropDownListGuid));
  const char *p="";
  if (ddl) p = ddl->getSelectedText();
  return MAKE_SCRIPT_STRING(p);
}

scriptVar DropDownListScriptController::DropDownList_onSelect(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar id, scriptVar hover) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, dropDownListController, id, hover);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, id, hover);
}

/*void*/ scriptVar DropDownListScriptController::DropDownList_setListHeight(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar h) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  if (ddl) {
    ddl->setListHeight(GET_SCRIPT_INT(h));
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar DropDownListScriptController::DropDownList_openList(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  if (ddl) {
    ddl->openList();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar DropDownListScriptController::DropDownList_closeList(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  if (ddl) {
    ddl->closeList();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar DropDownListScriptController::DropDownList_setItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar lotsofitems) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  if (ddl) {
    ddl->setItems(GET_SCRIPT_STRING(lotsofitems));
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar DropDownListScriptController::DropDownList_addItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar text) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  int retval = 0;
  if (ddl) {
    retval = ddl->addItem(GET_SCRIPT_STRING(text));
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar DropDownListScriptController::DropDownList_delItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar id) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  if (ddl) {
    ddl->delItem(GET_SCRIPT_INT(id));
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar DropDownListScriptController::DropDownList_findItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar text) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  int retval = 0;
  if (ddl) {
    retval = ddl->findItem(GET_SCRIPT_STRING(text));
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar DropDownListScriptController::DropDownList_getNumItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  int retval = 0;
  if (ddl) {
    retval = ddl->getNumItems();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar DropDownListScriptController::DropDownList_selectItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar id, /*int*/ scriptVar hover) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  if (ddl) {
    ddl->selectItem(GET_SCRIPT_INT(id), GET_SCRIPT_INT(hover));
  }
  RETURN_SCRIPT_VOID;
}

/*String*/ scriptVar DropDownListScriptController::DropDownList_getItemText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar id) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  const char *retval = "";
  if (ddl) {
    retval = ddl->getItemText(GET_SCRIPT_INT(id));
  }
  return MAKE_SCRIPT_STRING(retval);
}

/*int*/ scriptVar DropDownListScriptController::DropDownList_getSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  int retval = 0;
  if (ddl) {
    retval = ddl->getSelected();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*String*/ scriptVar DropDownListScriptController::DropDownList_getSelectedText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  const char *retval = "";
  if (ddl) {
    retval = ddl->getSelectedText();
  }
  return MAKE_SCRIPT_STRING(retval);
}

/*String*/ scriptVar DropDownListScriptController::DropDownList_getCustomText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  const char *retval="";
  if (ddl) {
    retval = ddl->getCustomText();
  }
  return MAKE_SCRIPT_STRING(retval);
}

/*void*/ scriptVar DropDownListScriptController::DropDownList_deleteAllItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  if (ddl) {
    ddl->deleteAllItems();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar DropDownListScriptController::DropDownList_setNoItemText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar txt) {
  SCRIPT_FUNCTION_INIT
  DropDownList *ddl = static_cast<DropDownList *>(o->vcpu_getInterface(dropDownListGuid));
  if (ddl) {
    ddl->setNoItemText(GET_SCRIPT_STRING(txt));
  }
  RETURN_SCRIPT_VOID;
}

