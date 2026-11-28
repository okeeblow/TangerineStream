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
#ifndef _EDITORI_H
#define _EDITORI_H

#include "editor.h"

#include "../bfc/wnds/itemlistwnd.h"
#include "../bfc/autobitmap.h"
#include "../bfc/appcmds.h"

#include "../common/script/objcontroller.h"

class Playlist;
class PlaylistEntry;
class EditWndString;
class svc_plDir;
class JumperHook;

class EditorN : public Editor {
public:
  virtual Dependent *getDependencyPtr()=0;
  virtual int setPlaylistByHandle(PlaylistHandle handle, const RECT *src)=0;
  virtual Playlist *_getPlaylist()=0;
  virtual void _scrollToY(int y)=0;
  virtual int _getScrollY()=0;
  virtual int _itemSelected(int pos)=0;
  virtual void _ensureVisible(int pos)=0;
  virtual void _enqueueItem(int pos, int force_key)=0;
  
  RECVS_DISPATCH;
};

#include "../bfc/appcmds.h"
#include "../common/contextmenu.h"

#define EDITORI_PARENT ItemListWnd
class EditorI : public EDITORI_PARENT, public EditorN,
  public DependentViewerI, private AppCmdsI
{
public:
  EditorI();
  virtual ~EditorI();

  static const char *xuiobject_getXmlTag() { return "PlaylistEditor"; }

  Dependent *getDependencyPtr() { return this; }//
  virtual void *dependent_getInterface(const GUID *classguid);

  virtual void *getInterface(GUID interface_guid);

  virtual int onInit();
//CUT  virtual int onRightButtonDown();
  virtual void onVScrollToggle(BOOL set);

  virtual DragItemI *itemlistwnd_getDragItem(int x, int y);
  virtual DragItemI *itemlistwnd_getSecondDragItem(int n);
  virtual void itemlistwnd_addCustomContextMenuCommands(ContextMenu *cm);
  virtual void itemlistwnd_contextMenuResult(int res);

  virtual int onAcceleratorEvent(const char *name);

  // adds PlaylistEntry*'s to dragging
  virtual int addMoreDragTypes(int pos);

  virtual int dragEnter(RootWnd *sourceWnd);
  virtual int dragOver(int x, int y, RootWnd *sourceWnd);
  virtual int dragLeave(RootWnd *sourceWnd);
  virtual int dragDrop(RootWnd *sourceWnd, int x, int y);
  virtual int dragComplete(int success);

  virtual int checkDragTypes(RootWnd *sourceWnd);

  virtual void onExternalDropBegin();
  virtual void onExternalDropDirScan(const char *dirname);
  virtual void onExternalDropEnd();

  virtual void onSetName();

  virtual void onItemSelection(int itemnum, int selected);

  virtual int onPaint(Canvas *canvas);

  virtual int onLeftButtonUp(int x, int y);
  virtual int wantHScroll() { return 0; }

  // from ItemListWnd
  virtual void onItemChange(int pos, const char *playstring);
  virtual void onItemDel(int pos, const char *playstring);

  int setPlaylist(Playlist *playlist);
  virtual int setPlaylistByHandle(PlaylistHandle handle, const RECT *src=NULL);
  Playlist *getPlaylist(int create_if_none=FALSE);
  void setPlaylistByHandleDeferred(PlaylistHandle handle);

  // class Editor bridges
  virtual Playlist *_getPlaylist() { return getPlaylist(FALSE); }
  virtual void _scrollToY(int y) { ScrlBkgWnd::scrollToY(y); }
  virtual int _getScrollY() { return ScrlBkgWnd::getScrollY(); }
  virtual int _itemSelected(int pos) { return getItemSelected(pos); }
  virtual void _ensureVisible(int pos) { ensureItemVisible(pos); }
  virtual void _enqueueItem(int pos, int force_key) { enqueueItem(pos, force_key); }

  void onPlaylistUnload(Playlist *playlist);

protected:
  void resetPlaylist();

public:
  int fakeDoubleClick(int pos);

  void deleteSelected();

  int selectDups();
  int selectDisabled();

  void setJumpText(const char *txt);

  int selectAll();
  int deselectAll();
  int invertSelection();
  void setSelectionEnd(int pos);

  virtual void appcmds_onCommand(int id, const RECT *buttonRect, int which_btn);

  static void createNewEditor(PlaylistHandle hand, int showpos=-1);

protected:
  void setPlaylistName();
  void _setPlaylistName();

  virtual int viewer_onItemDeleted(Dependent *item);
  virtual int viewer_onEvent(Dependent *item, const GUID *classguid, int hint1, int hint2, void *ptr, int ptrlen);
  virtual void metacb_onItemDel(const char *playstring);

  virtual void onDelete();	// handle delete requests
  virtual void onDoubleClick(int itemnum);	// double-click on an item
  virtual void onLeftClick(int itemnum);	// Left-click on item

  virtual void userRender(int pos, const char *playstring, Canvas &c, RECT &r, LPARAM lParam);

  virtual const char *convertlParam(LPARAM lParam);
  virtual void userColumnToText(int pos, const char *playstring, LPARAM lParam, char *str, int maxlen);

  virtual COLORREF getTextColor(LPARAM lParam);
  virtual COLORREF getBgColor(LPARAM lParam);
  virtual int getSelected(LPARAM lParam);

  virtual int acceptExternalDrops() { return 1; }	// override this

  virtual int onAction(const char *action, const char *param=NULL, int x=-1, int y=-1, int p1=0, int p2=0, void *data=NULL, int datalen=0, RootWnd *source=NULL);

  void enqueueItem(int pos, int force_key=0);

  void setColumns(const char *val);
  void applySetColumns();

private:
  Playlist *playlist;
  RECT invalidRect;
  int dragInsertPos;
  PlaylistEntry *context_entry;
  AutoSkinBitmap marker;	// the marker for playback queue
  AutoSkinBitmap dropind;	// the drop indicator
  int cwidth, twidth;
  String jumptext;
  int jumpoffset;
  bool noItemSelDraw;
  svc_plDir *pldir;

// XuiObject
public:
  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);
private:
  int isLocked();

  enum {
    EDITOR_SOMETHING
  };
  int myxuihandle;
  int need_reset;
  int showmodified, showlocked;//whether to show "(modified)" and "(locked)"
  int ismain;
  int islocked;//use isLocked()
  int removeondelete;
  int enqueueonclick;

  int need_length_reset;
  stdtimevalms last_setplname;

  String columns_list;
  String last_clicked_column;
  int reversed_last_time;

  virtual int onDeferredCallback(int p1, int p2);

  void resetAppCmds();
};

class EditorXuiSvc : public XuiObjectSvc2<EditorI> {};

#define LINK_0V(Z, Y) \
scriptVar \
static script_##Y##(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) { \
  SCRIPT_FUNCTION_INIT \
  Z *obj = static_cast<Z *>(o->vcpu_getInterface(Z::scriptobject_getClassGuid())); \
  if (obj) obj->Y(); \
  RETURN_SCRIPT_VOID; \
}

//class EditorController : public ScriptObjectControllerT<Editor, GuiObject> {
//public:
//  LINK_0V(Editor, onItemSelected);
//};

#define ITEMLISTCOLUMN_METATAGSORT_PARENT ItemListColumn_MetaTag
class ItemListColumn_MetaTagSort : public ItemListColumn_MetaTag {
public:
  ItemListColumn_MetaTagSort(const char *tag, int center=0, const char *label=NULL);
  virtual int onHeaderClick();
  virtual int customDrawHeader(Canvas *c, RECT *cr);

private:
  int reverse, show_arrow;
  AutoSkinBitmap down_arrow, up_arrow;
};

class ItemListColumn_Enabled : public ItemListColumn {
public:
  ItemListColumn_Enabled();
  virtual void render(int pos, const char *playstring, Canvas &c, RECT &r);
  virtual int onHeaderClick();
  virtual int onColumnLeftClick(int pos);
  virtual int customDrawHeader(Canvas *c, RECT *cr);
  virtual void columnToText(int pos, const char *playstring, char *str, int maxlen);

private:
  int nextval;
  AutoSkinBitmap checked, unchecked;
};

#endif
