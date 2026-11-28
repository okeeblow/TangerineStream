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
#include "xuilist.h"
#include "../studio/api.h"

#include "../bfc/svc_enum.h"
#include "../bfc/paramparser.h"
#include "../common/script/scriptguid.h"
#include "../studio/services/svc_textfeed.h"

// The temporary memory buffer to hold our string returns.
String GuiListScriptController::staticStr;


// -----------------------------------------------------------------------
char ScriptListXuiObjectStr[] = "List"; // This is the xml tag
char ScriptListXuiSvcName[] = "List XuiObject Service";


// -----------------------------------------------------------------------
ScriptList::ScriptList() {
  getScriptObject()->vcpu_setInterface(guilistGuid, (void *)static_cast<ScriptList *>(this));
  getScriptObject()->vcpu_setClassName("GuiList"); // this is the script class name
  getScriptObject()->vcpu_setController(guiListController);


  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "items", SCRIPTLIST_SETITEMS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "multiselect", SCRIPTLIST_SETMULTISELECT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "autodeselect", SCRIPTLIST_SETAUTODESELECT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "select", SCRIPTLIST_SELECT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "feed", SCRIPTLIST_FEED, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "hoverselect", SCRIPTLIST_HOVERSELECT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "sort", SCRIPTLIST_SORT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "selectonupdown", SCRIPTLIST_SELECTONUPDOWN, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "numcolumns", SCRIPTLIST_NUMCOLUMNS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "columnwidths", SCRIPTLIST_COLUMNWIDTHS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "columnlabels", SCRIPTLIST_COLUMNLABELS, XUI_ATTRIBUTE_IMPLIED);
  setPreventMultipleSelection(1);
  setVirtual(0);
  feed = NULL;
  xmlnumcolumns = -1;
  last_numcolumns = 0x80000000; // go ahead and try and be equal to that.
  getGuiObject()->guiobject_getScriptObject()->vcpu_setInterface(listGuid, (void *)this);
}

// -----------------------------------------------------------------------
ScriptList::~ScriptList() {
  closeFeed();
}

// -----------------------------------------------------------------------
int ScriptList::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTLIST_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case SCRIPTLIST_SETITEMS:
      items = value;
      fillFromParams();
#ifdef WASABI_COMPILE_CONFIG
      if (getGuiObject()->guiobject_hasCfgAttrib())
        selectFromConfig();
#endif
      break;
    case SCRIPTLIST_SETMULTISELECT:
      multiselect = ATOI(value);
      break;
    case SCRIPTLIST_SETAUTODESELECT:
      setWantAutoDeselect(ATOI(value));
      break;
    case SCRIPTLIST_SELECT: {
      int i = selectEntry(value);
      if (i != -1) 
        ensureItemVisible(i);
      else
        selectFirstEntry();
      break;
    }
    case SCRIPTLIST_FEED: {
      closeFeed();
      openFeed(value);
      break;
    }
    case SCRIPTLIST_HOVERSELECT: {
      setHoverSelect(ATOI(value));
      break;
    }
    case SCRIPTLIST_SORT: {
      setAutoSort(ATOI(value));
      break;
    }
    case SCRIPTLIST_SELECTONUPDOWN: {
      setSelectOnUpDown(ATOI(value));
      break;
    }
    case SCRIPTLIST_NUMCOLUMNS: {
      xmlnumcolumns = ATOI(value);
      setNumColumns();
      break;
    }
    case SCRIPTLIST_COLUMNWIDTHS: {
      columnwidths = value;
      setColumnWidths();
      break;
    }
    case SCRIPTLIST_COLUMNLABELS: {
      columnlabels = value;
      setColumnLabels();
      break;
    }
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
int ScriptList::onInit() {
  SCRIPTLIST_PARENT::onInit();

  last_numcolumns = 0x80000000;
  setNumColumns();  // Sets widths and labels if necessary

  setPreventMultipleSelection(!multiselect);

//  fillFromParams(); // done by setNumColumns();
  return 1;
}

/*
    Moved to script-oriented section
// -----------------------------------------------------------------------
void ScriptList::onDoubleClick(int itemnum) {
#ifdef WASABI_COMPILE_CONFIG
  saveToConfig();
#endif
}
*/

/*
    Moved to script-oriented section
// -----------------------------------------------------------------------
void ScriptList::onItemSelection(int itemnum, int selected) {
  SCRIPTLIST_PARENT::onItemSelection(itemnum, selected);
#ifdef WASABI_COMPILE_CONFIG
  saveToConfig();
#endif
}
*/


// -----------------------------------------------------------------------
int ScriptList::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source) {
  SCRIPTLIST_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, source);
  if (STRCASEEQL(action, "select_all")) {
    selectAll(0);
#ifdef WASABI_COMPILE_CONFIG
    saveToConfig();
#endif
  }
  if (STRCASEEQL(action, "deselect_all")) {
    deselectAll(0);
#ifdef WASABI_COMPILE_CONFIG
    saveToConfig();
#endif
  }
  if (STRCASEEQL(action, "get_selection")) {
    if (source != NULL) {
      String res("");
      for (int i=0;i<getNumItems();i++) {
        if (getItemSelected(i)) {
          if (!res.isempty()) res += ";";
          res += getSubitemText(i, 0);
        }
      }
      sendAction(source, "set_selection", res);
    }
  }
  return 1;
}

void ScriptList::onSetVisible(int i) {
  SCRIPTLIST_PARENT::onSetVisible(i);
}

#ifdef WASABI_COMPILE_CONFIG
// -----------------------------------------------------------------------
int ScriptList::onReloadConfig() {
  SCRIPTLIST_PARENT::onReloadConfig();
  selectFromConfig();
  return 1;
}

// -----------------------------------------------------------------------
void ScriptList::saveToConfig() {
  String res("");
  for (int i=0;i<getNumItems();i++) {
    if (getItemSelected(i)) {
      if (!res.isempty()) res += ";";
      res += getSubitemText(i, 0);
    }
  }
  getGuiObject()->guiobject_setCfgString(res);
}

// -----------------------------------------------------------------------
void ScriptList::selectFromConfig() {
  deselectAll(0);
  const char *p = getGuiObject()->guiobject_getCfgString();
  if (p != NULL) {
    ParamParser pp(p);
    for (int i=0;i<pp.getNumItems();i++)
      selectEntry(pp.enumItem(i), 0);
  }
}
#endif

// -----------------------------------------------------------------------
int ScriptList::selectEntry(const char *e, int cb) {
  for (int i=0;i<getNumItems();i++) {
    const char *si = getSubitemText(i, 0);
    if (STRCASEEQLSAFE(si, e)) {
      setSelected(i, 1, cb);
      return i;
    }
  }
  return -1;
}

// -----------------------------------------------------------------------
void ScriptList::fillFromParams() {
  deleteAllItems();
  if (!items.isempty()) {
    ParamParser pp(items);
    if (xmlnumcolumns == -1) {
      // OLD WAY
      for (int i=0;i<pp.getNumItems();i++)
        addItem(pp.enumItem(i), (LPARAM)NULL);
    } else {
      // NEW WAY
      int i, n = pp.getNumItems();
      for (i = 0; i < n; i++) {
        String row = pp.enumItem(i);
        ParamParser rp(row, ",");
        addItem(rp.enumItem(0), (LPARAM)NULL);
        int j, m = rp.getNumItems();
        for (j = 1; j < m; j++) {
          setSubItem(i, j, rp.enumItem(j));
        }
      }
    }
  }
}

// -----------------------------------------------------------------------
void ScriptList::selectEntries(const char *entries, int cb) {
  ParamParser pp(entries);
  for (int i=0;i<pp.getNumItems();i++)
    selectEntry(pp.enumItem(i), cb);
}

// -----------------------------------------------------------------------
void ScriptList::setColumnWidths() {
  // don't bother if there's no value.
  if (columnwidths.len()) {
    ParamParser pp(columnwidths);
    int i, n = MIN(pp.getNumItems(), getNumColumns());  // whichever is less.
    for (i = 0; i < n; i++) {
      ListColumn *column = getColumn(i);
      if (column) {
        column->setWidth(ATOI(pp.enumItem(i)));
      }
    }
  }
}

// -----------------------------------------------------------------------
void ScriptList::setColumnLabels() {
  // don't bother if there's no value.
  if (columnlabels.len()) {
    ParamParser pp(columnlabels);
    int i, n = MIN(pp.getNumItems(), getNumColumns());  // whichever is less.
    for (i = 0; i < n; i++) {
      ListColumn *column = getColumn(i);
      if (column) {
        column->setLabel(pp.enumItem(i));
      }
    }
  }
}

// -----------------------------------------------------------------------
void ScriptList::setNumColumns() {
  if (last_numcolumns == xmlnumcolumns) return;

  if (xmlnumcolumns == -1) {
    // the old way.
    insertColumn(new ListColumn("", TRUE));
  } else {
    // delete all columns.
    int i, n = getNumColumns();
    for (i = 0; i < n; i++) {
      this->delColumnByPos(0);
    }
    // create new ones.
    ParamParser cw(columnwidths);
    int nw = cw.getNumItems();
    ParamParser cl(columnlabels);
    int nl = cl.getNumItems();
    for (i = 0; i < xmlnumcolumns; i++) {
      const char *collabel = "";
      int colwidth = -1;  // magic value for "be dynamic"
      if (i < nl) {
        collabel = cl.enumItem(i);
      }
      if (i < nw) {
        colwidth = ATOI(cw.enumItem(i));
      }
      ListColumn *pCol = new ListColumn(collabel, (colwidth < 0));
      if (colwidth >= 0) {
        pCol->setWidth(colwidth);
      }
      insertColumn(pCol);
    }
    fillFromParams();
  }

  last_numcolumns = xmlnumcolumns;
}

// -----------------------------------------------------------------------
void ScriptList::openFeed(const char *feedid) {
  if (STRCASEEQL(feedid, last_feed)) return;
  feed = TextFeedEnum(feedid).getFirst();
  if (feed != NULL) {
    viewer_addViewItem(feed->getDependencyPtr());
  }
  last_feed = feedid;
}

// -----------------------------------------------------------------------
void ScriptList::closeFeed() {
  if (feed) {
    viewer_delViewItem(feed->getDependencyPtr());
    SvcEnum::release(feed);
  }
  feed = NULL;
  last_feed = "";
}

// -----------------------------------------------------------------------
int ScriptList::viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param, void *ptr, int ptrlen) {
  if (feed && feed->getDependencyPtr() == item) {
    if (event == svc_textFeed::Event_TEXTCHANGE) {
      setXuiParam(myxuihandle, SCRIPTLIST_SETITEMS, "items", (const char *)ptr);
      return 1;
    }
  }
  return 0;
}


// -----------------------------------------------------------------------
// Callback methods that send hooks into the Script system
void ScriptList::onSelectAll() {
  SCRIPTLIST_PARENT::onSelectAll();
  GuiListScriptController::guilist_onSelectAll(SCRIPT_CALL, getScriptObject());
}

void ScriptList::onDelete() {
  SCRIPTLIST_PARENT::onDelete();
  GuiListScriptController::guilist_onDelete(SCRIPT_CALL, getScriptObject());
}

void ScriptList::onDoubleClick(int itemnum) {
  SCRIPTLIST_PARENT::onDoubleClick(itemnum);
  GuiListScriptController::guilist_onDoubleClick(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(itemnum));
#ifdef WASABI_COMPILE_CONFIG
  saveToConfig();
#endif
}

void ScriptList::onLeftClick(int itemnum) {
  SCRIPTLIST_PARENT::onLeftClick(itemnum);
  GuiListScriptController::guilist_onLeftClick(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(itemnum));
}

void ScriptList::onSecondLeftClick(int itemnum) {
  SCRIPTLIST_PARENT::onSecondLeftClick(itemnum);
  GuiListScriptController::guilist_onSecondLeftClick(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(itemnum));
}

int ScriptList::onRightClick(int itemnum) {
  SCRIPTLIST_PARENT::onRightClick(itemnum);
  scriptVar v = GuiListScriptController::guilist_onRightClick(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(itemnum));
  if ((v.type != SCRIPT_VOID) && (v.type != SCRIPT_OBJECT) && (v.type != SCRIPT_STRING)) { 
    return GET_SCRIPT_BOOLEAN(v);
  }
  return 0;
}

int ScriptList::onColumnDblClick(int col, int x, int y) {
  SCRIPTLIST_PARENT::onColumnDblClick(col, x, y);
  scriptVar v = GuiListScriptController::guilist_onColumnDblClick(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(col), MAKE_SCRIPT_INT(y), MAKE_SCRIPT_INT(x));
  if ((v.type != SCRIPT_VOID) && (v.type != SCRIPT_OBJECT) && (v.type != SCRIPT_STRING)) {
    return GET_SCRIPT_BOOLEAN(v);
  }
  return 0;
}

int ScriptList::onColumnLabelClick(int col, int x, int y) {
  SCRIPTLIST_PARENT::onColumnLabelClick(col, x, y);
  scriptVar v = GuiListScriptController::guilist_onColumnLabelClick(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(col), MAKE_SCRIPT_INT(y), MAKE_SCRIPT_INT(x));
  if ((v.type != SCRIPT_VOID) && (v.type != SCRIPT_OBJECT) && (v.type != SCRIPT_STRING)) { 
    return GET_SCRIPT_BOOLEAN(v);
  }
  return 1; // don't ask me, that's what ListWnd does.
}

void ScriptList::onItemSelection(int itemnum, int selected) {
  SCRIPTLIST_PARENT::onItemSelection(itemnum, selected);
  GuiListScriptController::guilist_onItemSelection(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(itemnum), MAKE_SCRIPT_INT(selected));
#ifdef WASABI_COMPILE_CONFIG
  saveToConfig();
#endif
}



// -----------------------------------------------------------------------
// Script Object

GuiListScriptController _guiListController;
GuiListScriptController *guiListController = &_guiListController;

// -- Functions table -------------------------------------
function_descriptor_struct GuiListScriptController::exportedFunction[] = {
  {"getNumItems",        0, (void*)GuiListScriptController::guilist_getNumItems },
  {"getWantAutoDeselect", 0, (void*)guilist_getWantAutoDeselect },
  {"setWantAutoDeselect", 1, (void*)guilist_setWantAutoDeselect },
  {"onSetVisible", 1, (void*)guilist_onSetVisible },
  {"setAutoSort", 1, (void*)guilist_setAutoSort },
  {"next", 0, (void*)guilist_next },
  {"selectCurrent", 0, (void*)guilist_selectCurrent },
  {"selectFirstEntry", 0, (void*)guilist_selectFirstEntry },
  {"previous", 0, (void*)guilist_previous },
  {"pagedown", 0, (void*)guilist_pagedown },
  {"pageup", 0, (void*)guilist_pageup },
  {"home", 0, (void*)guilist_home },
  {"end", 0, (void*)guilist_end },
  {"reset", 0, (void*)guilist_reset },
  {"addColumn", 3, (void*)guilist_addColumn },
  {"getNumColumns", 0, (void*)guilist_getNumColumns },
  {"getColumnWidth", 1, (void*)guilist_getColumnWidth },
  {"setColumnWidth", 2, (void*)guilist_setColumnWidth },
  {"getColumnLabel", 1, (void*)guilist_getColumnLabel },
  {"setColumnLabel", 2, (void*)guilist_setColumnLabel },
  {"getColumnNumeric", 1, (void*)guilist_getColumnNumeric },
  {"setColumnDynamic", 2, (void*)guilist_setColumnDynamic },
  {"isColumnDynamic", 1, (void*)guilist_isColumnDynamic },
  {"setMinimumSize", 1, (void*)guilist_setMinimumSize },
  {"addItem", 1, (void*)guilist_addItem },
  {"insertItem", 2, (void*)guilist_insertItem },
  {"getLastAddedItemPos", 0, (void*)guilist_getLastAddedItemPos },
  {"setSubItem", 3, (void*)guilist_setSubItem },
  {"deleteAllItems", 0, (void*)guilist_deleteAllItems },
  {"deleteByPos", 1, (void*)guilist_deleteByPos },
  {"getItemLabel", 2, (void*)guilist_getItemLabel },
  {"setItemLabel", 2, (void*)guilist_setItemLabel },
  {"getItemSelected", 1, (void*)guilist_getItemSelected },
  {"isItemFocused", 1, (void*)guilist_isItemFocused },
  {"getItemFocused", 0, (void*)guilist_getItemFocused },
  {"setItemFocused", 1, (void*)guilist_setItemFocused },
  {"ensureItemVisible", 1, (void*)guilist_ensureItemVisible },
  {"invalidateColumns", 0, (void*)guilist_invalidateColumns },
  {"scrollAbsolute", 1, (void*)guilist_scrollAbsolute },
  {"scrollRelative", 1, (void*)guilist_scrollRelative },
  {"scrollLeft", 1, (void*)guilist_scrollLeft },
  {"scrollRight", 1, (void*)guilist_scrollRight },
  {"scrollUp", 1, (void*)guilist_scrollUp },
  {"scrollDown", 1, (void*)guilist_scrollDown },
  {"getSubitemText", 2, (void*)guilist_getSubitemText },
  {"getFirstItemSelected", 0, (void*)guilist_getFirstItemSelected },
  {"getNextItemSelected", 1, (void*)guilist_getNextItemSelected },
  {"selectAll", 0, (void*)guilist_selectAll },
  {"deselectAll", 0, (void*)guilist_deselectAll },
  {"invertSelection", 0, (void*)guilist_invertSelection },
  {"invalidateItem", 1, (void*)guilist_invalidateItem },
  {"getFirstItemVisible", 0, (void*)guilist_getFirstItemVisible },
  {"getLastItemVisible", 0, (void*)guilist_getLastItemVisible },
  {"setFontSize", 1, (void*)guilist_setFontSize },
  {"getFontSize", 0, (void*)guilist_getFontSize },
  {"jumpToNext", 1, (void*)guilist_jumpToNext },
  {"scrollToItem", 1, (void*)guilist_scrollToItem },
  {"resort", 0, (void*)guilist_resort },
  {"getSortDirection", 0, (void*)guilist_getSortDirection },
  {"getSortColumn", 0, (void*)guilist_getSortColumn },
  {"setSortColumn", 1, (void*)guilist_setSortColumn },
  {"setSortDirection", 1, (void*)guilist_setSortDirection },
  {"getItemCount", 0, (void*)guilist_getItemCount },
  {"setSelectionStart", 1, (void*)guilist_setSelectionStart },
  {"setSelectionEnd", 1, (void*)guilist_setSelectionEnd },
  {"setSelected", 2, (void*)guilist_setSelected },
  {"toggleSelection", 2, (void*)guilist_toggleSelection },
  {"getHeaderHeight", 0, (void*)guilist_getHeaderHeight },
  {"getPreventMultipleSelection", 0, (void*)guilist_getPreventMultipleSelection },
  {"setPreventMultipleSelection", 1, (void*)guilist_setPreventMultipleSelection },
  {"moveItem", 2, (void*)guilist_moveItem },
  {"onSelectAll", 0, (void*)guilist_onSelectAll },
  {"onDelete", 0, (void*)guilist_onDelete },
  {"onDoubleClick", 1, (void*)guilist_onDoubleClick },
  {"onLeftClick", 1, (void*)guilist_onLeftClick },
  {"onSecondLeftClick", 1, (void*)guilist_onSecondLeftClick },
  {"onRightClick", 1, (void*)guilist_onRightClick },
  {"onColumnDblClick", 3, (void*)guilist_onColumnDblClick },
  {"onColumnLabelClick", 3, (void*)guilist_onColumnLabelClick },
  {"onItemSelection", 2, (void*)guilist_onItemSelection },
};

ScriptObject *GuiListScriptController::instantiate() {
  ScriptList *sp = new ScriptList;
  ASSERT(sp != NULL);
  return sp->getScriptObject();
}

void GuiListScriptController::destroy(ScriptObject *o) {
  ScriptList *sp = static_cast<ScriptList *>(o->vcpu_getInterface(guilistGuid));
  ASSERT(sp != NULL);
  delete sp;
}

void *GuiListScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for guilists yet
}

void GuiListScriptController::deencapsulate(void *o) {
}

int GuiListScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *GuiListScriptController::getExportedFunctions() { 
  return exportedFunction; 
}

/*int*/ scriptVar GuiListScriptController::guilist_getNumItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int a = 0;
  if (sp) a = sp->getNumItems();
  return MAKE_SCRIPT_INT(a);
}

/*int*/ scriptVar GuiListScriptController::guilist_getWantAutoDeselect(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int a = 0;
  if (sp) {
    sp->wantAutoDeselect();
  }
  return MAKE_SCRIPT_INT(a);
}

/*void*/ scriptVar GuiListScriptController::guilist_setWantAutoDeselect(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar want) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _want = GET_SCRIPT_INT(want);
    sp->setWantAutoDeselect(_want);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_onSetVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar show) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _show = GET_SCRIPT_INT(show);
    sp->onSetVisible(_show);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_setAutoSort(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar dosort) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _dosort = GET_SCRIPT_INT(dosort);
    sp->setAutoSort(_dosort);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_next(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->next();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_selectCurrent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->selectCurrent();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_selectFirstEntry(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->selectFirstEntry();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_previous(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->previous();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_pagedown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->pagedown();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_pageup(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->pageup();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_home(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->home();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_end(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->end();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_reset(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->reset();
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_addColumn(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar name, /*int*/ scriptVar width, /*int*/ scriptVar numeric) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    String _name = GET_SCRIPT_STRING(name);
    int _width = GET_SCRIPT_INT(width);
    int _numeric = GET_SCRIPT_INT(numeric);
    retval = sp->addColumn(_name, _width, _numeric);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getNumColumns(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getNumColumns();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getColumnWidth(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar column) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _column = GET_SCRIPT_INT(column);
    retval = sp->getColumnWidth(_column);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_setColumnWidth(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar column, /*int*/ scriptVar newwidth) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _column = GET_SCRIPT_INT(column);
    int _newwidth = GET_SCRIPT_INT(newwidth);
    ListColumn *c = sp->getColumn(_column);
    if (c) {
      c->setWidth(_newwidth);
    }
  }
  RETURN_SCRIPT_VOID;
}

/*String*/ scriptVar GuiListScriptController::guilist_getColumnLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar column) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  const char * retval = 0;
  if (sp) {
    int _column = GET_SCRIPT_INT(column);
    ListColumn *c = sp->getColumn(_column);
    if (c) {
      retval = c->getLabel();
    }
  }
  return MAKE_SCRIPT_STRING(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_setColumnLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar column, /*String*/ scriptVar newlabel) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _column = GET_SCRIPT_INT(column);
    String _newlabel = GET_SCRIPT_STRING(newlabel);
    ListColumn *c = sp->getColumn(_column);
    if (c) {
      c->setLabel(_newlabel);
    }
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_getColumnNumeric(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar column) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _column = GET_SCRIPT_INT(column);
    ListColumn *c = sp->getColumn(_column);
    if (c) {
      retval = c->getNumeric();
    }
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_setColumnDynamic(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar column, /*int*/ scriptVar isdynamic) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _column = GET_SCRIPT_INT(column);
    int _isdynamic = GET_SCRIPT_INT(isdynamic);
    ListColumn *c = sp->getColumn(_column);
    if (c) {
      c->setDynamic(_isdynamic);
    }
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_isColumnDynamic(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar column) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _column = GET_SCRIPT_INT(column);
    ListColumn *c = sp->getColumn(_column);
    if (c) {
      retval = c->isDynamic();
    }
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_setMinimumSize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar size) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _size = GET_SCRIPT_INT(size);
    sp->setMinimumSize(_size);
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_addItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar label) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    String _label = GET_SCRIPT_STRING(label);
    retval = sp->addItem(_label, 0);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_insertItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos, /*String*/ scriptVar label) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    String _label = GET_SCRIPT_STRING(label);
    retval = sp->insertItem(_pos, _label, 0);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getLastAddedItemPos(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getLastAddedItemPos();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_setSubItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos, /*int*/ scriptVar subpos, /*String*/ scriptVar txt) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    int _subpos = GET_SCRIPT_INT(subpos);
    String _txt = GET_SCRIPT_STRING(txt);
    sp->setSubItem(_pos, _subpos, _txt);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_deleteAllItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->deleteAllItems();
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_deleteByPos(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    retval = sp->deleteByPos(_pos);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*String*/ scriptVar GuiListScriptController::guilist_getItemLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos, /*int*/ scriptVar subpos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  char retval[255] = { 0 };
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    int _subpos = GET_SCRIPT_INT(subpos);
    sp->getItemLabel(_pos, _subpos, retval, 254);
  }
  staticStr = retval;
  return MAKE_SCRIPT_STRING(staticStr);
}

/*void*/ scriptVar GuiListScriptController::guilist_setItemLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos, /*String*/ scriptVar text) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    String _text = GET_SCRIPT_STRING(text);
    sp->setItemLabel(_pos, _text);
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_getItemSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    retval = sp->getItemSelected(_pos);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_isItemFocused(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    retval = sp->getItemFocused(_pos);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getItemFocused(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getItemFocused();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_setItemFocused(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    sp->setItemFocused(_pos);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_ensureItemVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    sp->ensureItemVisible(_pos);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_invalidateColumns(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->invalidateColumns();
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_scrollAbsolute(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar x) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _x = GET_SCRIPT_INT(x);
    retval = sp->scrollAbsolute(_x);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_scrollRelative(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar x) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _x = GET_SCRIPT_INT(x);
    retval = sp->scrollRelative(_x);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_scrollLeft(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar lines) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _lines = GET_SCRIPT_INT(lines);
    sp->scrollLeft();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_scrollRight(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar lines) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _lines = GET_SCRIPT_INT(lines);
    sp->scrollRight();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_scrollUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar lines) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _lines = GET_SCRIPT_INT(lines);
    sp->scrollUp();
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_scrollDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar lines) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _lines = GET_SCRIPT_INT(lines);
    sp->scrollDown();
  }
  RETURN_SCRIPT_VOID;
}

/*String*/ scriptVar GuiListScriptController::guilist_getSubitemText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos, /*int*/ scriptVar subpos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  const char *retval = 0;
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    int _subpos = GET_SCRIPT_INT(subpos);
    retval = sp->getSubitemText(_pos, _subpos);
  }
  return MAKE_SCRIPT_STRING(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getFirstItemSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getFirstItemSelected();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getNextItemSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar lastpos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _lastpos = GET_SCRIPT_INT(lastpos);
    retval = sp->getNextItemSelected(_lastpos);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_selectAll(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->selectAll();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_deselectAll(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->deselectAll();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_invertSelection(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->invertSelection();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_invalidateItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    retval = sp->invalidateItem(_pos);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getFirstItemVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getFirstItemVisible();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getLastItemVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getLastItemVisible();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_setFontSize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar size) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _size = GET_SCRIPT_INT(size);
    retval = sp->setFontSize(_size);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getFontSize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getFontSize();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_jumpToNext(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*char*/ scriptVar c) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    char _c = GET_SCRIPT_INT(c);
    sp->jumpToNext(_c);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_scrollToItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    sp->scrollToItem(_pos);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_resort(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    sp->resort();
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_getSortDirection(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getSortDirection();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getSortColumn(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getSortColumn();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_setSortColumn(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar col) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _col = GET_SCRIPT_INT(col);
    sp->setSortColumn(_col);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_setSortDirection(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar dir) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _dir = GET_SCRIPT_INT(dir);
    sp->setSortDirection(_dir);
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_getItemCount(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getItemCount();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_setSelectionStart(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    sp->setSelectionStart(_pos);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_setSelectionEnd(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    sp->setSelectionEnd(_pos);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_setSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos, /*int*/ scriptVar selected) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    int _selected = GET_SCRIPT_INT(selected);
    sp->setSelected(_pos, _selected);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_toggleSelection(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar pos, /*int*/ scriptVar setfocus) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _pos = GET_SCRIPT_INT(pos);
    int _setfocus = GET_SCRIPT_INT(setfocus);
    sp->toggleSelection(_pos, _setfocus);
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar GuiListScriptController::guilist_getHeaderHeight(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getHeaderHeight();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_getPreventMultipleSelection(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    retval = sp->getPreventMultipleSelection();
  }
  return MAKE_SCRIPT_INT(retval);
}

/*int*/ scriptVar GuiListScriptController::guilist_setPreventMultipleSelection(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar val) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  int retval = 0;
  if (sp) {
    int _val = GET_SCRIPT_INT(val);
    retval = sp->setPreventMultipleSelection(_val);
  }
  return MAKE_SCRIPT_INT(retval);
}

/*void*/ scriptVar GuiListScriptController::guilist_moveItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar from, /*int*/ scriptVar to) {
  SCRIPT_FUNCTION_INIT
  ScriptList *sp = static_cast<ScriptList*>(o->vcpu_getInterface(guilistGuid));
  if (sp) {
    int _from = GET_SCRIPT_INT(from);
    int _to = GET_SCRIPT_INT(to);
    sp->moveItem(_from, _to);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar GuiListScriptController::guilist_onSelectAll(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiListController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*void*/ scriptVar GuiListScriptController::guilist_onDelete(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiListController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*void*/ scriptVar GuiListScriptController::guilist_onDoubleClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar itemnum) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiListController, itemnum);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, itemnum);
}

/*void*/ scriptVar GuiListScriptController::guilist_onLeftClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar itemnum) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiListController, itemnum);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, itemnum);
}

/*void*/ scriptVar GuiListScriptController::guilist_onSecondLeftClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar itemnum) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiListController, itemnum);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, itemnum);
}

/*int*/ scriptVar GuiListScriptController::guilist_onRightClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar itemnum) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiListController, itemnum);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, itemnum);
}

/*int*/ scriptVar GuiListScriptController::guilist_onColumnDblClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar col, /*int*/ scriptVar x, /*int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS3(o, guiListController, col, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT3(o, col, x, y);
}

/*int*/ scriptVar GuiListScriptController::guilist_onColumnLabelClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar col, /*int*/ scriptVar x, /*int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS3(o, guiListController, col, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT3(o, col, x, y);
}

/*void*/ scriptVar GuiListScriptController::guilist_onItemSelection(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar itemnum, /*int*/ scriptVar selected) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiListController, itemnum, selected);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, itemnum, selected);
}

