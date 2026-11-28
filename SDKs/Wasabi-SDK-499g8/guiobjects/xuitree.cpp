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
#include "xuitree.h"
#include "../studio/api.h"

#include "../bfc/svc_enum.h"
#include "../bfc/hierarchyparser.h"
#include "../common/script/scriptguid.h"
#include "../studio/services/svc_textfeed.h"



// The temporary memory buffer to hold our string returns.
String GuiTreeScriptController::staticStr;

class ScriptTreeItem;

// -----------------------------------------------------------------------
// class TreeItemScript -- This is the tree item type inserted into
//    the tree if a script or XML piece is inserting items into the tree.
class TreeItemScript : public TreeItem {
public:
  TreeItemScript(const char *label = NULL, ScriptTreeItem *_scriptitem = NULL) : scriptitem(_scriptitem), TreeItem(label) {
  }
  virtual ~TreeItemScript() {
  }
  virtual void onTreeAdd() {
    if (scriptitem) TreeItemScriptController::treeitem_onTreeAdd(SCRIPT_CALL, scriptitem->getScriptObject());
  }
  virtual void onTreeRemove() {
    if (scriptitem) TreeItemScriptController::treeitem_onTreeRemove(SCRIPT_CALL, scriptitem->getScriptObject());
  }
  virtual void onSelect() {
    if (scriptitem) TreeItemScriptController::treeitem_onSelect(SCRIPT_CALL, scriptitem->getScriptObject());
  }
  virtual void onDeselect() {
    if (scriptitem) TreeItemScriptController::treeitem_onDeselect(SCRIPT_CALL, scriptitem->getScriptObject());
  }
  virtual int onLeftDoubleClick() {
    scriptVar retval;
    if (scriptitem) retval = TreeItemScriptController::treeitem_onLeftDoubleClick(SCRIPT_CALL, scriptitem->getScriptObject());
    if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) return 0;
    return GET_SCRIPT_INT(retval);
  }
  virtual int onRightDoubleClick() { 
    scriptVar retval;
    if (scriptitem) retval = TreeItemScriptController::treeitem_onRightDoubleClick(SCRIPT_CALL, scriptitem->getScriptObject());
    if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) return 0;
    return GET_SCRIPT_INT(retval); 
  }
  // return 1 if you eat the key
  virtual int onChar(UINT key) { 
    scriptVar retval;
    if (scriptitem) retval = TreeItemScriptController::treeitem_onChar(SCRIPT_CALL, scriptitem->getScriptObject(), MAKE_SCRIPT_INT(key));
    if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) return 0;
    return GET_SCRIPT_INT(retval);
  }
  // these are called after the expand/collapse happens
  virtual void onExpand() { 
    if (scriptitem) TreeItemScriptController::treeitem_onExpand(SCRIPT_CALL, scriptitem->getScriptObject());
  }
  virtual void onCollapse() { 
    if (scriptitem) TreeItemScriptController::treeitem_onCollapse(SCRIPT_CALL, scriptitem->getScriptObject());
  }
  virtual int onBeginLabelEdit() {
    scriptVar retval;
    if (scriptitem) retval = TreeItemScriptController::treeitem_onBeginLabelEdit(SCRIPT_CALL, scriptitem->getScriptObject());
    int retv = 0;
    if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
      retv = -1;
    }
    if (!retv) {
      retv = GET_SCRIPT_INT(retval);
    }
    if (retv < 1) {
      retv = TreeItem::onBeginLabelEdit();
    }
    return retv;
  }
  virtual int onEndLabelEdit(const char *newlabel) {
    scriptVar retval;
    if (scriptitem) retval = TreeItemScriptController::treeitem_onEndLabelEdit(SCRIPT_CALL, scriptitem->getScriptObject(), MAKE_SCRIPT_STRING(newlabel));
    int retv = 0;
    if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
      retv = -1;
    }
    if (!retv) {
      retv = GET_SCRIPT_INT(retval);
    }
    if (retv < 1) {
      retv = TreeItem::onEndLabelEdit(newlabel);
    }
    return retv;
  }
  virtual int onContextMenu(int x, int y) {
    scriptVar retval;
    if (scriptitem) retval = TreeItemScriptController::treeitem_onContextMenu(SCRIPT_CALL, scriptitem->getScriptObject(), MAKE_SCRIPT_INT(x), MAKE_SCRIPT_INT(y) );
    int retv = 0;
    if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
      retv = -1;
    }
    if (!retv) {
      retv = GET_SCRIPT_INT(retval);
    }
    if (retv < 1) {
      retv = TreeItem::onContextMenu(x, y);
    }
    return retv;
  }
protected:
  ScriptTreeItem *scriptitem;
};


// -----------------------------------------------------------------------
char ScriptTreeXuiObjectStr[] = "Tree"; // This is the xml tag
char ScriptTreeXuiSvcName[] = "Tree XuiObject Service";


// -----------------------------------------------------------------------
ScriptTree::ScriptTree() {
  getScriptObject()->vcpu_setInterface(guitreeGuid, (void *)static_cast<ScriptTree *>(this));
  getScriptObject()->vcpu_setClassName("GuiTree"); // this is the script class name
  getScriptObject()->vcpu_setController(guiTreeController);


  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "items", SCRIPTTREE_SETITEMS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "feed", SCRIPTTREE_FEED, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "sorted", SCRIPTTREE_SORTED, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "childtabs", SCRIPTTREE_CHILDTABS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "expandroot", SCRIPTTREE_EXPANDROOT, XUI_ATTRIBUTE_IMPLIED);
  
  feed = NULL;
  childtabs = 1;
  expandroot = 1;
}

// -----------------------------------------------------------------------
ScriptTree::~ScriptTree() {
  closeFeed();

  // Clean up the ScriptTreeItems owned by this guy.
  TreeItem *item = enumRootItem(0);
  while (item != NULL) {
    // Delete this item.
    ScriptTreeItem *dead = NULL;
    if (TISC::g_scriptitems.getItem(item, &dead) && (dead != NULL)) { // true if it found something.
      TISC::g_scriptitems.delItem(item);
      delete dead;
//      DebugString(" === deleting tree item (%08X, %08X, %08X)\n", item, dead, this);
      continue;
    } else {
//      DebugString(" !!! ORPHAN TREE ITEM (%08X, %08X, %08X)\n", item, 0, this);
    }
   
    // Figure out who the next item to process should be.

    // 1) Children first.
    TreeItem *child = item->getChild();
    if (child != NULL) {
      item = child;
    } else {
      // 2) Siblings next.
      TreeItem *sibling = item->getSibling();
      if (sibling != NULL) {
        item = sibling;
      } else {
        // 3) Zip up parent chain last.
        TreeItem *item_parent, *parent_sibling;
        item_parent = item->getParent();
        item = NULL; // at this point if we do not assign, we are NULL.
        while (item_parent != NULL) {
          parent_sibling = item_parent->getSibling();
          if (parent_sibling != NULL) {
            item = parent_sibling;
            break;
          }
          item_parent = item_parent->getParent();
        }
        // 4) Uhhh.... you're null.  All done.  Go home.
      }
    }
  }

/*

  // delete all of our script items from g_scriptitems
  int i = 0;
  TreeItem *next = enumRootItem(0);
  while (next) { // go through all of our items
    ScriptTreeItem *dead = NULL;
    if (TISC::g_scriptitems.getItem(next, &dead) && (dead != NULL)) { // true if it found something.
      TISC::g_scriptitems.delItem(next);
      delete dead;
//      DebugString(" === deleting tree item (%08X, %08X)\n", next, dead);
    } else {
//      DebugString(" !!! ORPHAN TREE ITEM (%08X, %08X)\n", next, i);
    }
    TreeItem *next = enumAllItems(i++);
  }

*/
  // some items will wind up leaked into g_scriptitems, most likely.
}

// -----------------------------------------------------------------------
int ScriptTree::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTTREE_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case SCRIPTTREE_SETITEMS:
      items = value;
      fillFromParams();
#ifdef WASABI_COMPILE_CONFIG
      if (getGuiObject()->guiobject_hasCfgAttrib())
        selectFromConfig();
#endif
      break;
    case SCRIPTTREE_FEED: {
      closeFeed();
      openFeed(value);
      break;
    }
    case SCRIPTTREE_SORTED: {
      setSorted(ATOI(value));
      break;
    }
    case SCRIPTTREE_CHILDTABS: {
      childtabs = ATOI(value);
      break;
    }
    case SCRIPTTREE_EXPANDROOT: {
      expandRoot(ATOI(value));
      break;
    }
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
int ScriptTree::onInit() {
  SCRIPTTREE_PARENT::onInit();

  fillFromParams();
  return 1;
}

// -----------------------------------------------------------------------
int ScriptTree::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source) {
  SCRIPTTREE_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, source);
  if (STRCASEEQL(action, "get_selection")) {
    if (source != NULL) {
      String res("");

      // Hmmmmm..... multiselection trees?

      sendAction(source, "set_selection", res);
    }
  }
  return 1;
}

// -----------------------------------------------------------------------
ScriptTreeItem *ScriptTree::bindScriptTreeItem(TreeItem *item) {
  ASSERT(item != NULL);
  // find this tree item in our map of tree items.
  ScriptTreeItem *si = NULL;
  TISC::g_scriptitems.getItem(item, &si);
  if (si == NULL) {
    // if there was no scriptobject in our map already, make one
    // for this tree item and place it in our map.
    si = new ScriptTreeItem(item, this);
    TISC::g_scriptitems.addItem(item, si);
//    DebugString(" === NEW TREE ITEM (%08X, %08X, %08X)\n", item, si, this);
  } else {
//    DebugString(" === found tree item (%08X, %08X, %08X)\n", item, si, this);
  }
  return si;
}

// -----------------------------------------------------------------------
int ScriptTree::destroyScriptTreeItem(ScriptTreeItem *item) {
  ASSERT(item != NULL);
  // find this tree item in our map of tree items.
  TreeItem *ti = item->getItem();
  if (ti) {
    ScriptTreeItem *check = NULL;
    TISC::g_scriptitems.getItem(ti, &check);  // this is a doublecheck on who owns who.
    if (check != NULL) {
      // remove the treeitem from the tree 
      this->removeTreeItem(ti); // (removes without deleting)
      // remove the scripttreeitem from the map
      TISC::g_scriptitems.delItem(ti);
      // and delete.(phew!  bomb disposal!)
      delete ti;
      delete item;
      return 1; // yes, we deleted it.
    }
  }
  return 0; // Not ours, don't wanna delete it.  Someone else can Deal With It.
}

// -----------------------------------------------------------------------
void ScriptTree::onSetVisible(int i) {
  SCRIPTTREE_PARENT::onSetVisible(i);
}

#ifdef WASABI_COMPILE_CONFIG
// -----------------------------------------------------------------------
int ScriptTree::onReloadConfig() {
  SCRIPTTREE_PARENT::onReloadConfig();
  selectFromConfig();
  return 1;
}

// -----------------------------------------------------------------------
void ScriptTree::saveToConfig() {
}

// -----------------------------------------------------------------------
void ScriptTree::selectFromConfig() {
}
#endif

// -----------------------------------------------------------------------
int ScriptTree::selectEntry(const char *e, int cb) {
  return -1;
}

// -----------------------------------------------------------------------
void ScriptTree::expandRoot(int val) {
  if (val) {
    expandroot = 1;
    int count;
    TreeItem *rootitem;
    for (count = 0,rootitem = enumRootItem(count); rootitem; rootitem = enumRootItem(++count)) {
      rootitem->expand();
    }
  } else {
    expandroot = 0;
  }
}

// -----------------------------------------------------------------------
void ScriptTree::fillFromHPNode(HPNode *node, TreeItem *parent) {
  // Go through the given node's children and add items for them
  // to the corresponding parent item.
  int i, n = node->getNumChildren();
  for (i = 0; i < n; i++) {
    // Here's a child node
    HPNode *child_node = static_cast<HPNode *>(node->enumChild(i));
    // Make a script-aware tree item and script item to correspond to it, labelled with the child_node's name
    ScriptTreeItem *si = new ScriptTreeItem;
    TreeItem *child_item = new TreeItemScript((*child_node)(), si);
    si->setItem(child_item);
    si->setScriptTree(this);
    // Add the script and tree items to the scriptitems map
    TISC::g_scriptitems.addItem(child_item, si);
//    DebugString(StringPrintf(" === NEW NODE ITEM (%08X, %08X, %08X)\n", child_item, si, this);
    // Add the child item to either ourselves or the given parent.
    addTreeItem(child_item, parent, getSorted(), childtabs);
    // And then continue to fill from that node.
    fillFromHPNode(child_node, child_item);
  }
}

// -----------------------------------------------------------------------
void ScriptTree::fillFromParams() {
  deleteAllItems();
  if (!items.isempty()) {
    HierarchyParser hierarchy(items);
    fillFromHPNode(hierarchy.rootNode());
  }
  // If we want our roots opened, do so now.
  expandRoot(expandroot);
}

// -----------------------------------------------------------------------
void ScriptTree::selectEntries(const char *entries, int cb) {
}

// -----------------------------------------------------------------------
void ScriptTree::openFeed(const char *feedid) {
  if (STRCASEEQL(feedid, last_feed)) return;
  feed = TextFeedEnum(feedid).getFirst();
  if (feed != NULL) {
    viewer_addViewItem(feed->getDependencyPtr());
    setXuiParam(myxuihandle, SCRIPTTREE_SETITEMS, "items", feed->getFeedText(feedid));
  }
  last_feed = feedid;
}

// -----------------------------------------------------------------------
void ScriptTree::closeFeed() {
  if (feed) {
    viewer_delViewItem(feed->getDependencyPtr());
    SvcEnum::release(feed);
  }
  feed = NULL;
  last_feed = "";
}

// -----------------------------------------------------------------------
int ScriptTree::viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param, void *ptr, int ptrlen) {
  if (feed == dynamic_guid_cast<svc_textFeed>(item, classguid)) {
    if (event == svc_textFeed::Event_TEXTCHANGE) {
      setXuiParam(myxuihandle, SCRIPTTREE_SETITEMS, "items", (const char *)ptr);
      return 1;
    }
  }
  return 0;
}

// -----------------------------------------------------------------------
// Callback methods that send hooks into the Script system

int ScriptTree::onLeftButtonDown(int x, int y) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onLeftButtonDown(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(x), MAKE_SCRIPT_INT(y) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
  }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
  }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onLeftButtonDown(x, y);
  }
  return retv;
}

int ScriptTree::onLeftButtonUp(int x, int y) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onLeftButtonUp(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(x), MAKE_SCRIPT_INT(y) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
  }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
  }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onLeftButtonUp(x, y);
  }
  return retv;
}

int ScriptTree::onRightButtonUp(int x, int y) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onRightButtonUp(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(x), MAKE_SCRIPT_INT(y) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onRightButtonUp(x, y);
    }
  return retv;
}

int ScriptTree::onMouseMove(int x, int y) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onMouseMove(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(x), MAKE_SCRIPT_INT(y) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onMouseMove(x, y);
    }
  return retv;
}

int ScriptTree::wantAutoContextMenu() {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_wantAutoContextMenu(SCRIPT_CALL, getScriptObject() );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::wantAutoContextMenu();
    }
  return retv;
}

int ScriptTree::onLeftButtonDblClk(int x, int y) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onLeftButtonDblClk(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(x), MAKE_SCRIPT_INT(y) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onLeftButtonDblClk(x, y);
    }
  return retv;
}

int ScriptTree::onRightButtonDblClk(int x, int y) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onRightButtonDblClk(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(x), MAKE_SCRIPT_INT(y) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onRightButtonDblClk(x, y);
    }
  return retv;
}

int ScriptTree::onMouseWheelUp(int clicked, int lines) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onMouseWheelUp(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(clicked), MAKE_SCRIPT_INT(lines) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onMouseWheelUp(clicked, lines);
    }
  return retv;
}

int ScriptTree::onMouseWheelDown(int clicked, int lines) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onMouseWheelDown(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(clicked), MAKE_SCRIPT_INT(lines) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onMouseWheelDown(clicked, lines);
    }
  return retv;
}

int ScriptTree::onContextMenu(int x, int y) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onContextMenu(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(x), MAKE_SCRIPT_INT(y) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onContextMenu(x, y);
    }
  return retv;
}

int ScriptTree::onChar(char c) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onChar(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(c));
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onChar(c);
    }
  return retv;
}

int ScriptTree::onKeyDown(int keycode) {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onKeyDown(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(keycode) );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
    }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
    }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onKeyDown(keycode);
    }
  return retv;
}

void ScriptTree::onItemRecvDrop(TreeItem *item) {
  ScriptTreeItem *sti_item = bindScriptTreeItem(item);
  GuiTreeScriptController::guitree_onItemRecvDrop(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_OBJECT(sti_item->getScriptObject()) );
}

void ScriptTree::onLabelChange(TreeItem *item) {
  ScriptTreeItem *sti_item = bindScriptTreeItem(item);
  GuiTreeScriptController::guitree_onLabelChange(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_OBJECT(sti_item->getScriptObject()) );
}

void ScriptTree::onItemSelected(TreeItem *item) {
  ScriptTreeItem *sti_item = bindScriptTreeItem(item);
  GuiTreeScriptController::guitree_onItemSelected(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_OBJECT(sti_item->getScriptObject()) );
}

void ScriptTree::onItemDeselected(TreeItem *item) {
  ScriptTreeItem *sti_item = bindScriptTreeItem(item);
  GuiTreeScriptController::guitree_onItemDeselected(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_OBJECT(sti_item->getScriptObject()) );
}

int ScriptTree::onKillFocus() {
  scriptVar retval;
  retval = GuiTreeScriptController::guitree_onKillFocus(SCRIPT_CALL, getScriptObject() );
  int retv = 0;
  if ((retval.type == SCRIPT_VOID) || (retval.type == SCRIPT_OBJECT) || (retval.type == SCRIPT_STRING)) {
    retv = -1;
  }
  if (!retv) {
    retv = GET_SCRIPT_INT(retval);
  }
  if (retv < 1) {
    retv = SCRIPTTREE_PARENT::onKillFocus();
  }
  return retv;
}





// -----------------------------------------------------------------------
// Script Object

GuiTreeScriptController _guiTreeController;
GuiTreeScriptController *guiTreeController = &_guiTreeController;

// -- Functions table -------------------------------------
function_descriptor_struct GuiTreeScriptController::exportedFunction[] = {
  {"getNumRootItems",        0, (void*)GuiTreeScriptController::guitree_getNumRootItems },
  {"enumRootItem",           1, (void*)GuiTreeScriptController::guitree_enumRootItem },

  {"onLeftButtonDown",       2, (void*)GuiTreeScriptController::guitree_onLeftButtonDown }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);
  {"onLeftButtonUp",         2, (void*)GuiTreeScriptController::guitree_onLeftButtonUp }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);
  {"onRightButtonUp",        2, (void*)GuiTreeScriptController::guitree_onRightButtonUp }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);
  {"onMouseMove",            2, (void*)GuiTreeScriptController::guitree_onMouseMove }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);
  {"onWantAutoContextMenu",  0, (void*)GuiTreeScriptController::guitree_wantAutoContextMenu }, // );
  {"onLeftButtonDblClk",     2, (void*)GuiTreeScriptController::guitree_onLeftButtonDblClk }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);
  {"onRightButtonDblClk",    2, (void*)GuiTreeScriptController::guitree_onRightButtonDblClk }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);
  {"onMouseWheelUp",         2, (void*)GuiTreeScriptController::guitree_onMouseWheelUp }, // , /*Int*/ scriptVar clicked, /*Int*/ scriptVar lines);
  {"onMouseWheelDown",       2, (void*)GuiTreeScriptController::guitree_onMouseWheelDown }, // , /*Int*/ scriptVar clicked, /*Int*/ scriptVar lines);
  {"onContextMenu",          2, (void*)GuiTreeScriptController::guitree_onContextMenu }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);
  {"onChar",                 1, (void*)GuiTreeScriptController::guitree_onChar }, // , /*Int*/ scriptVar c);
  {"onKeyDown",              1, (void*)GuiTreeScriptController::guitree_onKeyDown }, // , /*Int*/ scriptVar keycode);
  {"onItemRecvDrop",         1, (void*)GuiTreeScriptController::guitree_onItemRecvDrop }, // , /*TreeItem*/ scriptVar item);
  {"onLabelChange",          1, (void*)GuiTreeScriptController::guitree_onLabelChange }, // , /*TreeItem*/ scriptVar item);
  {"onItemSelected",         1, (void*)GuiTreeScriptController::guitree_onItemSelected }, // , /*TreeItem*/ scriptVar item);
  {"onItemDeselected",       1, (void*)GuiTreeScriptController::guitree_onItemDeselected }, // , /*TreeItem*/ scriptVar item);
  {"onKillFocus",            0, (void*)GuiTreeScriptController::guitree_onKillFocus }, // );
  {"jumpToNext",             1, (void*)GuiTreeScriptController::guitree_jumpToNext }, // , /*Int*/ scriptVar c);
  {"ensureItemVisible",      1, (void*)GuiTreeScriptController::guitree_ensureItemVisible }, // , /*TreeItem*/ scriptVar item);
  {"getContentsWidth",       0, (void*)GuiTreeScriptController::guitree_getContentsWidth }, // );
  {"getContentsHeight",      0, (void*)GuiTreeScriptController::guitree_getContentsHeight }, // );
  {"addTreeItem",            4, (void*)GuiTreeScriptController::guitree_addTreeItem }, // , /*TreeItem*/ scriptVar item, /*TreeItem*/ scriptVar par, /*Int*/ scriptVar sorted, /*Int*/ scriptVar haschildtab);
  {"removeTreeItem",         1, (void*)GuiTreeScriptController::guitree_removeTreeItem }, // , /*TreeItem*/ scriptVar item);
  {"moveTreeItem",           2, (void*)GuiTreeScriptController::guitree_moveTreeItem }, // , /*TreeItem*/ scriptVar item, /*TreeItem*/ scriptVar newparent);
  {"deleteAllItems",         0, (void*)GuiTreeScriptController::guitree_deleteAllItems }, // );
  {"expandItem",             1, (void*)GuiTreeScriptController::guitree_expandItem }, // , /*TreeItem*/ scriptVar item);
  {"expandItemDeferred",     1, (void*)GuiTreeScriptController::guitree_expandItemDeferred }, // , /*TreeItem*/ scriptVar item);
  {"collapseItem",           1, (void*)GuiTreeScriptController::guitree_collapseItem }, // , /*TreeItem*/ scriptVar item);
  {"collapseItemDeferred",   1, (void*)GuiTreeScriptController::guitree_collapseItemDeferred }, // , /*TreeItem*/ scriptVar item);
  {"selectItem",             1, (void*)GuiTreeScriptController::guitree_selectItem }, // , /*TreeItem*/ scriptVar item);
  {"selectItemDeferred",     1, (void*)GuiTreeScriptController::guitree_selectItemDeferred }, // , /*TreeItem*/ scriptVar item);
  {"delItemDeferred",        1, (void*)GuiTreeScriptController::guitree_delItemDeferred }, // , /*TreeItem*/ scriptVar item);
  {"hiliteItem",             1, (void*)GuiTreeScriptController::guitree_hiliteItem }, // , /*TreeItem*/ scriptVar item);
  {"unhiliteItem",           1, (void*)GuiTreeScriptController::guitree_unhiliteItem }, // , /*TreeItem*/ scriptVar item);
  {"getCurItem",             0, (void*)GuiTreeScriptController::guitree_getCurItem }, // );
  {"hitTest",                2, (void*)GuiTreeScriptController::guitree_hitTest }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);
  {"editItemLabel",          1, (void*)GuiTreeScriptController::guitree_editItemLabel }, // , /*TreeItem*/ scriptVar item);
  {"cancelEditLabel",        1, (void*)GuiTreeScriptController::guitree_cancelEditLabel }, // , /*Int*/ scriptVar destroyit);
  {"setAutoEdit",            1, (void*)GuiTreeScriptController::guitree_setAutoEdit }, // , /*Int*/ scriptVar ae);
  {"getAutoEdit",            0, (void*)GuiTreeScriptController::guitree_getAutoEdit }, // );
  {"getByLabel",             2, (void*)GuiTreeScriptController::guitree_getByLabel }, // , /*TreeItem*/ scriptVar item, /*String*/ scriptVar  name);
  {"setSorted",              1, (void*)GuiTreeScriptController::guitree_setSorted }, // , /*Int*/ scriptVar dosort);
  {"getSorted",              0, (void*)GuiTreeScriptController::guitree_getSorted }, // );
  {"sortTreeItems",          0, (void*)GuiTreeScriptController::guitree_sortTreeItems }, // );
  {"getSibling",             1, (void*)GuiTreeScriptController::guitree_getSibling }, // , /*TreeItem*/ scriptVar item);
  {"setAutoCollapse",        1, (void*)GuiTreeScriptController::guitree_setAutoCollapse }, // , /*Int*/ scriptVar doautocollapse);
  {"setFontSize",            1, (void*)GuiTreeScriptController::guitree_setFontSize }, // , /*Int*/ scriptVar newsize);
  {"getFontSize",            0, (void*)GuiTreeScriptController::guitree_getFontSize }, // );
  {"getNumVisibleChildItems",1, (void*)GuiTreeScriptController::guitree_getNumVisibleChildItems }, // , /*TreeItem*/ scriptVar c);
  {"getNumVisibleItems",     0, (void*)GuiTreeScriptController::guitree_getNumVisibleItems }, // );
  {"enumVisibleItems",       1, (void*)GuiTreeScriptController::guitree_enumVisibleItems }, // , /*Int*/ scriptVar n);
  {"enumVisibleChildItems",  2, (void*)GuiTreeScriptController::guitree_enumVisibleChildItems }, // , /*TreeItem*/ scriptVar c, /*Int*/ scriptVar n);
  {"enumAllItems",           1, (void*)GuiTreeScriptController::guitree_enumAllItems }, // , /*Int*/ scriptVar n);
  {"getItemRectX",           1, (void*)GuiTreeScriptController::guitree_getItemRectX }, // , /*TreeItem*/ scriptVar item);
  {"getItemRectY",           1, (void*)GuiTreeScriptController::guitree_getItemRectY }, // , /*TreeItem*/ scriptVar item);
  {"getItemRectW",           1, (void*)GuiTreeScriptController::guitree_getItemRectW }, // , /*TreeItem*/ scriptVar item);
  {"getItemRectH",           1, (void*)GuiTreeScriptController::guitree_getItemRectH }, // , /*TreeItem*/ scriptVar item);
//  {"getItemFromPoint",       2, (void*)GuiTreeScriptController::guitree_getItemFromPoint }, // , /*Int*/ scriptVar x, /*Int*/ scriptVar y);

};

ScriptObject *GuiTreeScriptController::instantiate() {
  ScriptTree *sp = new ScriptTree;
  ASSERT(sp != NULL);
  return sp->getScriptObject();
}

void GuiTreeScriptController::destroy(ScriptObject *o) {
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  ASSERT(sp != NULL);
  delete sp;
}

void *GuiTreeScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for guitrees yet
}

void GuiTreeScriptController::deencapsulate(void *o) {
}

int GuiTreeScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *GuiTreeScriptController::getExportedFunctions() { 
  return exportedFunction; 
}

/*int*/ scriptVar GuiTreeScriptController::guitree_getNumRootItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree*>(o->vcpu_getInterface(guitreeGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->getNumRootItems();
  }
  return MAKE_SCRIPT_INT(a);
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_enumRootItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar which) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree*>(o->vcpu_getInterface(guitreeGuid));
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    int _which = GET_SCRIPT_INT(which);
    a = sp->enumRootItem(_which);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onLeftButtonDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, x, y);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onLeftButtonUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, x, y);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onRightButtonUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, x, y);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onMouseMove(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, x, y);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_wantAutoContextMenu(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onLeftButtonDblClk(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, x, y);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onRightButtonDblClk(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, x, y);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onMouseWheelUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar clicked, /*Int*/ scriptVar lines) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, clicked, lines);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, clicked, lines);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onMouseWheelDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar clicked, /*Int*/ scriptVar lines) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, clicked, lines);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, clicked, lines);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onContextMenu(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, x, y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, x, y);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onChar(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar c) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiTreeController, c);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, c);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onKeyDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar keycode) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiTreeController, keycode);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, keycode);
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_onItemRecvDrop(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiTreeController, item);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, item);
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_onLabelChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiTreeController, item);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, item);
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_onItemSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiTreeController, item);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, item);
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_onItemDeselected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiTreeController, item);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, item);
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_onKillFocus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

// -------------------------------------------------------------------------
/*Void*/ scriptVar GuiTreeScriptController::guitree_jumpToNext(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar c) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _c = GET_SCRIPT_INT(c);
  if (sp != NULL) {
    sp->jumpToNext(_c);
  }
  RETURN_SCRIPT_VOID;
}


/*Void*/ scriptVar GuiTreeScriptController::guitree_ensureItemVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->ensureItemVisible(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getContentsWidth(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->getContentsWidth();
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getContentsHeight(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->getContentsHeight();
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_addTreeItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item, scriptVar par, scriptVar sorted, scriptVar haschildtab) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_par = NULL;
  ScriptObject *so_par = GET_SCRIPT_OBJECT(par);
  if (so_par) {
    ScriptTreeItem *sti_par = static_cast<ScriptTreeItem *>(so_par->vcpu_getInterface(treeitemGuid));
    if (sti_par) {
      _par = sti_par->getItem();
    }
  }
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  int _haschildtab = GET_SCRIPT_INT(haschildtab);
  int _sorted = GET_SCRIPT_INT(sorted);
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    a = sp->addTreeItem(_item, _par, _sorted, _haschildtab);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_removeTreeItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->removeTreeItem(_item);
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_moveTreeItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item, scriptVar newparent) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_newparent = NULL;
  ScriptObject *so_newparent = GET_SCRIPT_OBJECT(newparent);
  if (so_newparent) {
    ScriptTreeItem *sti_newparent = static_cast<ScriptTreeItem *>(so_newparent->vcpu_getInterface(treeitemGuid));
    if (sti_newparent) {
      _newparent = sti_newparent->getItem();
    }
  }
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->moveTreeItem(_item, _newparent);
  }
  RETURN_SCRIPT_VOID;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_deleteAllItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  if (sp != NULL) {
    sp->deleteAllItems();
  }
  RETURN_SCRIPT_VOID;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_expandItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->expandItem(_item);
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_expandItemDeferred(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->expandItemDeferred(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_collapseItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->collapseItem(_item);
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_collapseItemDeferred(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->collapseItemDeferred(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_selectItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->selectItem(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_selectItemDeferred(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->selectItemDeferred(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_delItemDeferred(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->delItemDeferred(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_hiliteItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->hiliteItem(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_unhiliteItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->unhiliteItem(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_getCurItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    a = sp->getCurItem();
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_hitTest(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _y = GET_SCRIPT_INT(y);
  int _x = GET_SCRIPT_INT(x);
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    a = sp->hitTest(_x, _y);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_editItemLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  if (sp != NULL) {
    sp->editItemLabel(_item);
  }
  RETURN_SCRIPT_VOID;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_cancelEditLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar destroyit) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _destroyit = GET_SCRIPT_INT(destroyit);
  if (sp != NULL) {
    sp->cancelEditLabel(_destroyit);
  }
  RETURN_SCRIPT_VOID;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_setAutoEdit(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar ae) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _ae = GET_SCRIPT_INT(ae);
  if (sp != NULL) {
    sp->setAutoEdit(_ae);
  }
  RETURN_SCRIPT_VOID;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getAutoEdit(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->getAutoEdit();
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_getByLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item, /*String*/ scriptVar name) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  String _name = GET_SCRIPT_STRING(name);
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    a = sp->getByLabel(_item, _name);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_setSorted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar dosort) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _dosort = GET_SCRIPT_INT(dosort);
  if (sp != NULL) {
    sp->setSorted(_dosort);
  }
  RETURN_SCRIPT_VOID;
}

/*Int*/ scriptVar  GuiTreeScriptController::guitree_getSorted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    int a = NULL;
    a = sp->getSorted();
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_sortTreeItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  if (sp != NULL) {
    sp->sortTreeItems();
  }
  RETURN_SCRIPT_VOID;
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_getSibling(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    a = sp->getSibling(_item);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*Void*/ scriptVar GuiTreeScriptController::guitree_setAutoCollapse(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar doautocollapse) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _doautocollapse = GET_SCRIPT_INT(doautocollapse);
  if (sp != NULL) {
    sp->setAutoCollapse(_doautocollapse);
  }
  RETURN_SCRIPT_VOID;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_setFontSize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar newsize) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _newsize = GET_SCRIPT_INT(newsize);
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->setFontSize(_newsize);
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getFontSize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->getFontSize();
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getNumVisibleChildItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar c) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_c = NULL;
  ScriptObject *so_c = GET_SCRIPT_OBJECT(c);
  if (so_c) {
    ScriptTreeItem *sti_c = static_cast<ScriptTreeItem *>(so_c->vcpu_getInterface(treeitemGuid));
    if (sti_c) {
      _c = sti_c->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->getNumVisibleChildItems(_c);
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getNumVisibleItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    a = sp->getNumVisibleItems();
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_enumVisibleItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar n) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _n = GET_SCRIPT_INT(n);
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    a = sp->enumVisibleItems(_n);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_enumVisibleChildItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar c, scriptVar n) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_c = NULL;
  ScriptObject *so_c = GET_SCRIPT_OBJECT(c);
  if (so_c) {
    ScriptTreeItem *sti_c = static_cast<ScriptTreeItem *>(so_c->vcpu_getInterface(treeitemGuid));
    if (sti_c) {
      _c = sti_c->getItem();
    }
  }
  int _n = GET_SCRIPT_INT(n);
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    a = sp->enumVisibleChildItems(_c, _n);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_enumAllItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar n) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _n = GET_SCRIPT_INT(n);
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    a = sp->enumAllItems(_n);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getItemRectX(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    RECT r;
    sp->getItemRect(_item, &r);
    a = r.left;
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getItemRectY(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    RECT r;
    sp->getItemRect(_item, &r);
    a = r.top;
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getItemRectW(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    RECT r;
    sp->getItemRect(_item, &r);
    a = r.left - r.right;
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

/*Int*/ scriptVar GuiTreeScriptController::guitree_getItemRectH(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar item) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  TreeItem *_item = NULL;
  ScriptObject *so_item = GET_SCRIPT_OBJECT(item);
  if (so_item) {
    ScriptTreeItem *sti_item = static_cast<ScriptTreeItem *>(so_item->vcpu_getInterface(treeitemGuid));
    if (sti_item) {
      _item = sti_item->getItem();
    }
  }
  scriptVar retval = MAKE_SCRIPT_INT(0);
  if (sp != NULL) {
    int a = 0;
    RECT r;
    sp->getItemRect(_item, &r);
    a = r.bottom - r.top;
    retval = MAKE_SCRIPT_INT(a);
  }
  return retval;
}

#if 0 // Not implemented in TreeWnd, dammit.
/*TreeItem*/ scriptVar GuiTreeScriptController::guitree_getItemFromPoint(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y) {
  SCRIPT_FUNCTION_INIT
  ScriptTree *sp = static_cast<ScriptTree *>(o->vcpu_getInterface(guitreeGuid));
  int _y = GET_SCRIPT_INT(y);
  int _x = GET_SCRIPT_INT(x);
  scriptVar retval = MAKE_SCRIPT_OBJECT(NULL);
  if (sp != NULL) {
    TreeItem *a = NULL;
    POINT p = {_x, _y};
    a = sp->getItemFromPoint(&p);
    if (a != NULL) {
      ScriptTreeItem *item = sp->bindScriptTreeItem(a);
      if (item != NULL) {
        retval = MAKE_SCRIPT_OBJECT(item->getScriptObject());
      }
    }
  }
  return retval;
}
#endif

// -----------------------------------------------------------------------
// Script Object For The Tree Item

ScriptTreeItem::ScriptTreeItem(TreeItem *_item, ScriptTree *_tree) : item(_item), tree(_tree), SCRIPTTREEITEM_SCRIPTPARENT() {
  getScriptObject()->vcpu_setInterface(treeitemGuid, (void *)static_cast<ScriptTreeItem *>(this));
  getScriptObject()->vcpu_setClassName("TreeItem");
  getScriptObject()->vcpu_setController(treeItemController);
}

ScriptTreeItem::~ScriptTreeItem() {
}

int ScriptTreeItem::getNumChildren() {
  ASSERT(item);
  return item->getNumChildren();
}

void ScriptTreeItem::setLabel(const char *label) {
  ASSERT(item);
  item->setLabel(label);
}

const char *ScriptTreeItem::getLabel() {
  ASSERT(item);
  return item->getLabel();
}

void ScriptTreeItem::ensureVisible() {
  ASSERT(item);
  item->ensureVisible();
}

TreeItem *ScriptTreeItem::getNthChild(int nth) {
  ASSERT(item);
  return item->getNthChild(nth);
}

TreeItem *ScriptTreeItem::getChild() {
  ASSERT(item);
  return item->getChild();
}

TreeItem *ScriptTreeItem::getChildSibling(TreeItem *_item) {
  ASSERT(item);
  return item->getChildSibling(_item);
}

TreeItem *ScriptTreeItem::getSibling() {
  ASSERT(item);
  return item->getSibling();
}

TreeItem *ScriptTreeItem::getParent() {
  ASSERT(item);
  return item->getParent();
}

void ScriptTreeItem::editLabel() {
  ASSERT(item);
  item->editLabel();
}

BOOL ScriptTreeItem::hasSubItems() {
  ASSERT(item);
  return item->hasSubItems();
}

void ScriptTreeItem::setSorted(int issorted) {
  ASSERT(item);
  item->setSorted(issorted);
}

void ScriptTreeItem::setChildTab(int haschildtab) {
  ASSERT(item);
  item->setChildTab(haschildtab);
}

BOOL ScriptTreeItem::isSorted() {
  ASSERT(item);
  return item->isSorted();
}

BOOL ScriptTreeItem::isCollapsed() {
  ASSERT(item);
  return item->isCollapsed();
}

BOOL ScriptTreeItem::isExpanded() {
  ASSERT(item);
  return item->isExpanded();
}

void ScriptTreeItem::invalidate() {
  ASSERT(item);
  item->invalidate();
}

BOOL ScriptTreeItem::isSelected() {
  ASSERT(item);
  return item->isSelected();
}

BOOL ScriptTreeItem::isHilited() {
  ASSERT(item);
  return item->isHilited();
}

void ScriptTreeItem::setHilited(BOOL ishilited) {
  ASSERT(item);
  item->setHilited(ishilited);
}

int ScriptTreeItem::collapse() {
  ASSERT(item);
  return item->collapse();
}

int ScriptTreeItem::expand() {
  ASSERT(item);
  return item->expand();
}

#if 0
//    This was never implemented!
void ScriptTreeItem::setCurrent(BOOL tf) {
  ASSERT(item);
  item->setCurrent(tf);
}
#endif

TreeWnd *ScriptTreeItem::getTree() {
  ASSERT(item);
  return item->getTree();
}

// -----------------------------------------------------------------------
// Script Controller For The Tree Item

TreeItemScriptController _treeItemController;
TreeItemScriptController *treeItemController = &_treeItemController;

// -- Functions table -------------------------------------
function_descriptor_struct TreeItemScriptController::exportedFunction[] = {
  {"getNumChildren",    0, (void*)TreeItemScriptController::treeitem_getNumChildren },
  {"setLabel",          1, (void*)TreeItemScriptController::treeitem_setLabel },
  {"getLabel",          0, (void*)TreeItemScriptController::treeitem_getLabel },
  {"ensureVisible",     0, (void*)TreeItemScriptController::treeitem_ensureVisible },
  {"getNthChild",       1, (void*)TreeItemScriptController::treeitem_getNthChild },
  {"getChild",          0, (void*)TreeItemScriptController::treeitem_getChild },
  {"getChildSibling",   1, (void*)TreeItemScriptController::treeitem_getChildSibling },
  {"getSibling",        0, (void*)TreeItemScriptController::treeitem_getSibling },
  {"getParent",         0, (void*)TreeItemScriptController::treeitem_getParent },
  {"editLabel",         0, (void*)TreeItemScriptController::treeitem_editLabel },
  {"hasSubItems",       0, (void*)TreeItemScriptController::treeitem_hasSubItems },
  {"setSorted",         1, (void*)TreeItemScriptController::treeitem_setSorted },
  {"setChildTab",       1, (void*)TreeItemScriptController::treeitem_setChildTab },
  {"isSorted",          0, (void*)TreeItemScriptController::treeitem_isSorted },
  {"isCollapsed",       0, (void*)TreeItemScriptController::treeitem_isCollapsed },
  {"isExpanded",        0, (void*)TreeItemScriptController::treeitem_isExpanded },
  {"invalidate",        0, (void*)TreeItemScriptController::treeitem_invalidate },
  {"isSelected",        0, (void*)TreeItemScriptController::treeitem_isSelected },
  {"isHilited",         0, (void*)TreeItemScriptController::treeitem_isHilited },
  {"setHilited",        1, (void*)TreeItemScriptController::treeitem_setHilited },
  {"collapse",          0, (void*)TreeItemScriptController::treeitem_collapse },
  {"expand",            0, (void*)TreeItemScriptController::treeitem_expand },
  {"getTree",           0, (void*)TreeItemScriptController::treeitem_getTree },

  {"onTreeAdd",         0, (void*)TreeItemScriptController::treeitem_onTreeAdd },
  {"onTreeRemove",      0, (void*)TreeItemScriptController::treeitem_onTreeRemove },
  {"onSelect",          0, (void*)TreeItemScriptController::treeitem_onSelect },
  {"onDeselect",        0, (void*)TreeItemScriptController::treeitem_onDeselect },
  {"onLeftDoubleClick", 0, (void*)TreeItemScriptController::treeitem_onLeftDoubleClick },
  {"onRightDoubleClick",0, (void*)TreeItemScriptController::treeitem_onRightDoubleClick },
  {"onChar",            1, (void*)TreeItemScriptController::treeitem_onChar },
  {"onExpand",          0, (void*)TreeItemScriptController::treeitem_onExpand },
  {"onCollapse",        0, (void*)TreeItemScriptController::treeitem_onCollapse },
  {"onBeginLabelEdit",  0, (void*)TreeItemScriptController::treeitem_onBeginLabelEdit },
  {"onEndLabelEdit",    1, (void*)TreeItemScriptController::treeitem_onEndLabelEdit },
  {"onContextMenu",     2, (void*)TreeItemScriptController::treeitem_onContextMenu },

};

String TreeItemScriptController::staticStr;
ScriptTreeMap TreeItemScriptController::g_scriptitems;

ScriptObject *TreeItemScriptController::instantiate() {
  ScriptTreeItem *sp = new ScriptTreeItem;
  ASSERT(sp != NULL);
  TreeItem *child_item = new TreeItemScript("", sp);
  ASSERT(child_item != NULL);
  sp->setItem(child_item);
  TISC::g_scriptitems.addItem(child_item, sp);
  // We're not attached to a tree.  that's okay!
  return sp->getScriptObject();
}

// If the script asks to delete the item, delete the internal item as well.
// We tell the owning ScriptTree to remove this object.
void TreeItemScriptController::destroy(ScriptObject *o) {
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  ASSERT(sp != NULL);
  if (!sp->destroyScriptTreeItem()) {
    // Ask the owner tree to do it for us, but if not owned, we do it ourselves.
    if (sp->getItem()) {
      TISC::g_scriptitems.delItem(sp->getItem());
    }
    // AND we delete our item, since we're not a part of a tree that will do it for us.
    delete sp->getItem();
    delete sp;
  }
}

void *TreeItemScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for guitrees yet
}

void TreeItemScriptController::deencapsulate(void *o) {
}

int TreeItemScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *TreeItemScriptController::getExportedFunctions() { 
  return exportedFunction; 
}

/*int*/ scriptVar TreeItemScriptController::treeitem_getNumChildren(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->getNumChildren();
  }
  return MAKE_SCRIPT_INT(a);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_setLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar label) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  String _label = GET_SCRIPT_STRING(label);
  if (sp != NULL) {
    sp->setLabel(_label);
  }
  RETURN_SCRIPT_VOID;
}

/*String*/ scriptVar TreeItemScriptController::treeitem_getLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  if (sp != NULL) {
    staticStr = sp->getLabel();
  }
  return MAKE_SCRIPT_STRING(staticStr);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_ensureVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  if (sp != NULL) {
    sp->ensureVisible();
  }
  RETURN_SCRIPT_VOID;
}

/*TreeItem*/ scriptVar TreeItemScriptController::treeitem_getNthChild(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar nth) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int _nth = GET_SCRIPT_INT(nth);
  TreeItem *a = NULL;
  if (sp != NULL) {
    a = sp->getNthChild(_nth);
  }
  ScriptTree *tree = sp->getScriptTree();
  ScriptTreeItem *retval = NULL;
  if (tree && a) {
    retval = tree->bindScriptTreeItem(a);
  }
  if (retval) {
    return MAKE_SCRIPT_OBJECT(retval->getScriptObject());
  }
  return MAKE_SCRIPT_OBJECT(NULL); // Return NULL
}

/*TreeItem*/ scriptVar TreeItemScriptController::treeitem_getChild(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  TreeItem *a = NULL;
  if (sp != NULL) {
    a = sp->getChild();
  }
  ScriptTree *tree = sp->getScriptTree();
  ScriptTreeItem *retval = NULL;
  if (tree && a) {
    retval = tree->bindScriptTreeItem(a);
  }
  if (retval) {
    return MAKE_SCRIPT_OBJECT(retval->getScriptObject());
  }
  return MAKE_SCRIPT_OBJECT(NULL); // Return NULL
}

/*TreeItem*/ scriptVar TreeItemScriptController::treeitem_getChildSibling(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar _item) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  TreeItem *__item = NULL;
  ScriptObject *io = GET_SCRIPT_OBJECT(_item);
  if (io) {
    ScriptTreeItem *sio = static_cast<ScriptTreeItem *>(io->vcpu_getInterface(treeitemGuid));
    if (sio) {
      __item = sio->getItem();
    }
  }
  TreeItem *a = NULL;
  if (sp != NULL) {
    a = sp->getChildSibling(__item);
  }
  ScriptTree *tree = sp->getScriptTree();
  ScriptTreeItem *retval = NULL;
  if (tree && a) {
    retval = tree->bindScriptTreeItem(a);
  }
  if (retval) {
    return MAKE_SCRIPT_OBJECT(retval->getScriptObject());
  }
  return MAKE_SCRIPT_OBJECT(NULL); // Return NULL
}

/*TreeItem*/ scriptVar TreeItemScriptController::treeitem_getSibling(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  TreeItem *a = NULL;
  if (sp != NULL) {
    a = sp->getSibling();
  }
  ScriptTree *tree = sp->getScriptTree();
  ScriptTreeItem *retval = NULL;
  if (tree && a) {
    retval = tree->bindScriptTreeItem(a);
  }
  if (retval) {
    return MAKE_SCRIPT_OBJECT(retval->getScriptObject());
  }
  return MAKE_SCRIPT_OBJECT(NULL); // Return NULL
}

/*TreeItem*/ scriptVar TreeItemScriptController::treeitem_getParent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  TreeItem *a = NULL;
  if (sp != NULL) {
    a = sp->getParent();
  }
  ScriptTree *tree = sp->getScriptTree();
  ScriptTreeItem *retval = NULL;
  if (tree && a) {
    retval = tree->bindScriptTreeItem(a);
  }
  if (retval) {
    return MAKE_SCRIPT_OBJECT(retval->getScriptObject());
  }
  return MAKE_SCRIPT_OBJECT(NULL); // Return NULL
}

/*void*/ scriptVar TreeItemScriptController::treeitem_editLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  if (sp != NULL) {
    sp->editLabel();
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar TreeItemScriptController::treeitem_hasSubItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->hasSubItems();
  }
  return MAKE_SCRIPT_INT(a);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_setSorted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar issorted) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int _issorted = GET_SCRIPT_INT(issorted);
  if (sp != NULL) {
    sp->setSorted(_issorted);
  }
  RETURN_SCRIPT_VOID;
}

/*void*/ scriptVar TreeItemScriptController::treeitem_setChildTab(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar haschildtab) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int _haschildtab = GET_SCRIPT_INT(haschildtab);
  if (sp != NULL) {
    sp->setChildTab(_haschildtab);
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar TreeItemScriptController::treeitem_isSorted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->isSorted();
  }
  return MAKE_SCRIPT_INT(a);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_isCollapsed(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->isCollapsed();
  }
  return MAKE_SCRIPT_INT(a);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_isExpanded(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->isExpanded();
  }
  return MAKE_SCRIPT_INT(a);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_invalidate(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  if (sp != NULL) {
    sp->invalidate();
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar TreeItemScriptController::treeitem_isSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->isSelected();
  }
  return MAKE_SCRIPT_INT(a);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_isHilited(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->isHilited();
  }
  return MAKE_SCRIPT_INT(a);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_setHilited(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar ishilited) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int _ishilited = GET_SCRIPT_INT(ishilited);
  if (sp != NULL) {
    sp->setHilited(_ishilited);
  }
  RETURN_SCRIPT_VOID;
}

/*int*/ scriptVar TreeItemScriptController::treeitem_collapse(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->collapse();
  }
  return MAKE_SCRIPT_INT(a);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_expand(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int a = 0;
  if (sp != NULL) {
    a = sp->expand();
  }
  return MAKE_SCRIPT_INT(a);
}

#if 0
//    This was never implemented!
/*void*/ scriptVar TreeItemScriptController::treeitem_setCurrent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar tf) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  int _tf = GET_SCRIPT_INT(tf);
  if (sp != NULL) {
    sp->setCurrent(_tf);
  }
  RETURN_SCRIPT_VOID;
}
#endif

/*GuiTree*/ scriptVar TreeItemScriptController::treeitem_getTree(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptTreeItem *sp = static_cast<ScriptTreeItem *>(o->vcpu_getInterface(treeitemGuid));
  ScriptTree *a = NULL;
  ScriptObject *retval = NULL;
  if (sp != NULL) {
    a = sp->getScriptTree();
  }
  if (a) {
    retval = a->getScriptObject();    
  }
  return MAKE_SCRIPT_OBJECT(retval);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_onTreeAdd(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_onTreeRemove(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_onSelect(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_onDeselect(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_onLeftDoubleClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o)  {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_onRightDoubleClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o)  {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_onChar(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar _key)  {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiTreeController, _key);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, _key);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_onExpand(SCRIPT_FUNCTION_PARAMS, ScriptObject *o)  {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*void*/ scriptVar TreeItemScriptController::treeitem_onCollapse(SCRIPT_FUNCTION_PARAMS, ScriptObject *o)  {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_onBeginLabelEdit(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS0(o, guiTreeController);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT0(o);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_onEndLabelEdit(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar _newlabel) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, guiTreeController, _newlabel);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, _newlabel);
}

/*int*/ scriptVar TreeItemScriptController::treeitem_onContextMenu(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar _x, /*int*/ scriptVar _y) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS2(o, guiTreeController, _x, _y);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT2(o, _x, _y);
}
