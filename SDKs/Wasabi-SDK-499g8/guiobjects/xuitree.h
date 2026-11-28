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
#ifndef __XUITREE_H
#define __XUITREE_H

#include "../bfc/map.h"
#include "../bfc/wnds/treewnd.h"
#include "../common/script/objcontroller.h"
#include "../bfc/depend.h"

class HPNode;
class ScriptTreeItem;
class svc_textFeed;

#define SCRIPTTREE_PARENT TreeWnd

typedef Map<TreeItem *,ScriptTreeItem *>  ScriptTreeMap;

// -----------------------------------------------------------------------
class ScriptTree : public SCRIPTTREE_PARENT, public DependentViewerI {
  
  public:

    ScriptTree();
    virtual ~ScriptTree();

    virtual int onInit();

    int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);
    int onReloadConfig();

    virtual int viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param, void *ptr, int ptrlen);
    virtual void onSetVisible(int i);

    virtual int onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source);

    // Find a ScriptTreeItem to wrap a TreeItem
    ScriptTreeItem *bindScriptTreeItem(TreeItem *item);

    // Someone is deleting a ScriptTreeItem so we should stop tracking it.
    int destroyScriptTreeItem(ScriptTreeItem *item);

    // Transfer a TreeItem from our tree to a different tree (or global space)
    int transferScriptTreeItem(TreeItem *item, ScriptTree *tree);

    // Callback methods that send hooks into the Script system
    virtual int onLeftButtonDown(int x, int y);
    virtual int onLeftButtonUp(int x, int y);
    virtual int onRightButtonUp(int x, int y);
    virtual int onMouseMove(int x, int y);
    virtual int wantAutoContextMenu();
    virtual int onLeftButtonDblClk(int x, int y);
    virtual int onRightButtonDblClk(int x, int y);
    virtual int onMouseWheelUp(int clicked, int lines);
    virtual int onMouseWheelDown(int clicked, int lines);
    virtual int onContextMenu(int x, int y);
    virtual int onChar(char c);
    virtual int onKeyDown(int keycode);
    virtual void onItemRecvDrop(TreeItem *item);
    virtual void onLabelChange(TreeItem *item);
    virtual void onItemSelected(TreeItem *item);
    virtual void onItemDeselected(TreeItem *item);
    virtual int onKillFocus();



    // Valid XML Params for Tree
    enum {
      SCRIPTTREE_SETITEMS = 0,
      SCRIPTTREE_FEED,
      SCRIPTTREE_SORTED,
      SCRIPTTREE_CHILDTABS,
      SCRIPTTREE_EXPANDROOT,
    };

  private:

#ifdef WASABI_COMPILE_CONFIG
    void saveToConfig();
    void selectFromConfig();
#endif
    void expandRoot(int val);

    void fillFromParams();
    void fillFromHPNode(HPNode *node, TreeItem *parent = NULL);

    int selectEntry(const char *e, int cb=1);
    void selectEntries(const char *multientry, int cb=1);

    void openFeed(const char *feedid);
    void closeFeed();
    
    String items;
    int myxuihandle;
    int childtabs;
    int expandroot;
    svc_textFeed *feed;
    String last_feed;

//    ScriptTreeMap scriptitems;  
};

// -----------------------------------------------------------------------------------------------------
class GuiTreeScriptController : public ScriptObjectControllerI {
  public:

    virtual const char *getClassName() { return "GuiTree"; }
    virtual const char *getAncestorClassName() { return "GuiObject"; }
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid() { return guitreeGuid; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  public:
    static scriptVar /*int*/ guitree_getNumRootItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*TreeItem*/ guitree_enumRootItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ which);

    static /*Int*/ scriptVar guitree_onLeftButtonDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);
    static /*Int*/ scriptVar guitree_onLeftButtonUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);
    static /*Int*/ scriptVar guitree_onRightButtonUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);
    static /*Int*/ scriptVar guitree_onMouseMove(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);
    static /*Int*/ scriptVar guitree_wantAutoContextMenu(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*Int*/ scriptVar guitree_onLeftButtonDblClk(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);
    static /*Int*/ scriptVar guitree_onRightButtonDblClk(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);
    static /*Int*/ scriptVar guitree_onMouseWheelUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar clicked, /*Int*/ scriptVar lines);
    static /*Int*/ scriptVar guitree_onMouseWheelDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar clicked, /*Int*/ scriptVar lines);
    static /*Int*/ scriptVar guitree_onContextMenu(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);
    static /*Int*/ scriptVar guitree_onChar(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar c);
    static /*Int*/ scriptVar guitree_onKeyDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar keycode);
    static /*Void*/ scriptVar guitree_onItemRecvDrop(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_onLabelChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_onItemSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_onItemDeselected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Int*/ scriptVar guitree_onKillFocus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

    static /*Void*/ scriptVar guitree_jumpToNext(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar c);
    static /*Void*/ scriptVar guitree_ensureItemVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Int*/ scriptVar guitree_getContentsWidth(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*Int*/ scriptVar guitree_getContentsHeight(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*TreeItem*/ scriptVar guitree_addTreeItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item, /*TreeItem*/ scriptVar par, /*Int*/ scriptVar sorted, /*Int*/ scriptVar haschildtab);
    static /*Int*/ scriptVar guitree_removeTreeItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_moveTreeItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item, /*TreeItem*/ scriptVar newparent);
    static /*Void*/ scriptVar guitree_deleteAllItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*Int*/ scriptVar guitree_expandItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_expandItemDeferred(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Int*/ scriptVar guitree_collapseItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_collapseItemDeferred(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_selectItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_selectItemDeferred(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_delItemDeferred(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_hiliteItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_unhiliteItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*TreeItem*/ scriptVar guitree_getCurItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*TreeItem*/ scriptVar guitree_hitTest(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);
    static /*Void*/ scriptVar guitree_editItemLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_cancelEditLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar destroyit);
    static /*Void*/ scriptVar guitree_setAutoEdit(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar ae);
    static /*Int*/ scriptVar guitree_getAutoEdit(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*TreeItem*/ scriptVar guitree_getByLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item, /*String*/ scriptVar  name);
    static /*Void*/ scriptVar guitree_setSorted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar dosort);
    static /*Int*/ scriptVar  guitree_getSorted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*Void*/ scriptVar guitree_sortTreeItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*TreeItem*/ scriptVar guitree_getSibling(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Void*/ scriptVar guitree_setAutoCollapse(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar doautocollapse);
    static /*Int*/ scriptVar guitree_setFontSize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar newsize);
    static /*Int*/ scriptVar guitree_getFontSize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*Int*/ scriptVar guitree_getNumVisibleChildItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar c);
    static /*Int*/ scriptVar guitree_getNumVisibleItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*TreeItem*/ scriptVar guitree_enumVisibleItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar n);
    static /*TreeItem*/ scriptVar guitree_enumVisibleChildItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar c, /*Int*/ scriptVar n);
    static /*TreeItem*/ scriptVar guitree_enumAllItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar n);
    static /*Int*/ scriptVar guitree_getItemRectX(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Int*/ scriptVar guitree_getItemRectY(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Int*/ scriptVar guitree_getItemRectW(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
    static /*Int*/ scriptVar guitree_getItemRectH(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar item);
//    static /*TreeItem*/ scriptVar guitree_getItemFromPoint(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*Int*/ scriptVar x, /*Int*/ scriptVar y);

  private:
    static function_descriptor_struct exportedFunction[];

    static String staticStr;
};

extern GuiTreeScriptController *guiTreeController;


// -----------------------------------------------------------------------
extern char ScriptTreeXuiObjectStr[];
extern char ScriptTreeXuiSvcName[];
class ScriptTreeXuiSvc : public XuiObjectSvc<ScriptTree, ScriptTreeXuiObjectStr, ScriptTreeXuiSvcName> {};



// -----------------------------------------------------------------------
#define SCRIPTTREEITEM_SCRIPTPARENT RootObjectInstance

class ScriptTreeItem : public SCRIPTTREEITEM_SCRIPTPARENT {
public:
  ScriptTreeItem(TreeItem *_item = NULL, ScriptTree *_tree = NULL);
  virtual ~ScriptTreeItem();

  TreeItem *getItem() {return item;}
  void setItem(TreeItem *_item) {item = _item;}
  ScriptTree *getScriptTree() {return tree;}
  void setScriptTree(ScriptTree *_tree) {tree = _tree;}

  int destroyScriptTreeItem() {
    if (tree) return tree->destroyScriptTreeItem(this); // CAREFUL, WE GET OURSELVES DELETED HERE!!!!!!!!
    return 0;
  }

// These methods all thunk directly to the TreeItem
public:
  int getNumChildren();
  void setLabel(const char *label);
  const char *getLabel();
  void ensureVisible();
  TreeItem *getNthChild(int nth);
  TreeItem *getChild();
  TreeItem *getChildSibling(TreeItem *_item);
  TreeItem *getSibling();
  TreeItem *getParent();
  void editLabel();
  BOOL hasSubItems();
  void setSorted(int issorted);
  void setChildTab(int haschildtab);
  BOOL isSorted();
  BOOL isCollapsed();
  BOOL isExpanded();
  void invalidate();
  BOOL isSelected();
  BOOL isHilited();
  void setHilited(BOOL ishilited);
  int collapse();
  int expand();
//  void setCurrent(BOOL tf);
  TreeWnd *getTree();
  
private:
  ScriptTreeItem *bindScriptTreeItem(TreeItem *item) {
    if (tree) return tree->bindScriptTreeItem(item);
    return NULL;
  }
  
  TreeItem *item;
  ScriptTree *tree;
};


// -----------------------------------------------------------------------------------------------------
class TreeItemScriptController : public ScriptObjectControllerI {
  public:

    virtual const char *getClassName() { return "TreeItem"; }
    virtual const char *getAncestorClassName() { return "Object"; }
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(rootObjectGuid); }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid() { return treeitemGuid; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  public:
    static /*int*/ scriptVar treeitem_getNumChildren(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_setLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar label);
    static /*String*/ scriptVar treeitem_getLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_ensureVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*TreeItem*/ scriptVar treeitem_getNthChild(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar nth);
    static /*TreeItem*/ scriptVar treeitem_getChild(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*TreeItem*/ scriptVar treeitem_getChildSibling(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*TreeItem*/ scriptVar _item);
    static /*TreeItem*/ scriptVar treeitem_getSibling(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*TreeItem*/ scriptVar treeitem_getParent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_editLabel(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*int*/ scriptVar treeitem_hasSubItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_setSorted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar issorted);
    static /*void*/ scriptVar treeitem_setChildTab(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar haschildtab);
    static /*int*/ scriptVar treeitem_isSorted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*int*/ scriptVar treeitem_isCollapsed(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*int*/ scriptVar treeitem_isExpanded(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_invalidate(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*int*/ scriptVar treeitem_isSelected(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*int*/ scriptVar treeitem_isHilited(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_setHilited(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar ishilited);
    static /*int*/ scriptVar treeitem_collapse(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*int*/ scriptVar treeitem_expand(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
//    static /*void*/ scriptVar treeitem_setCurrent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar tf);
    static /*GuiTree*/ scriptVar treeitem_getTree(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

    static /*void*/ scriptVar treeitem_onTreeAdd(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_onTreeRemove(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_onSelect(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*void*/ scriptVar treeitem_onDeselect(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*int*/ scriptVar treeitem_onLeftDoubleClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) ;
    static /*int*/ scriptVar treeitem_onRightDoubleClick(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) ;
    static /*int*/ scriptVar treeitem_onChar(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar _key) ;
    static /*void*/ scriptVar treeitem_onExpand(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) ;
    static /*void*/ scriptVar treeitem_onCollapse(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) ;
    static /*int*/ scriptVar treeitem_onBeginLabelEdit(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static /*int*/ scriptVar treeitem_onEndLabelEdit(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*String*/ scriptVar _newlabel);
    static /*int*/ scriptVar treeitem_onContextMenu(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, /*int*/ scriptVar _x, /*int*/ scriptVar _y);


  private:
    static function_descriptor_struct exportedFunction[];

    static String staticStr;

    friend ScriptTree;
    static ScriptTreeMap g_scriptitems;  // items not living in trees are tracked here.

};

extern TreeItemScriptController *treeItemController;

#define TISC TreeItemScriptController 


#endif
