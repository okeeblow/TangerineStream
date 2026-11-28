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
#include "plitem.h"
#include "pledit.h"
#include "pldir.h"
#include "editor.h"
#include "playlisti.h"

#include "../studio/assert.h"
#include "../studio/api.h"
#include "../studio/playitem.h"
#include "../common/std.h"
#include "../common/metatags.h"
#include "../common/popup.h"
#include "../common/filename.h"
#include "../common/skinclr.h"
#include "../common/contextmenu.h"

#include "../common/dragitemi.h"

//#define SHOWNITEMS

static SkinColor plitemcolor("pledit.dir.plitem");

int PEItem::onBeginDrag(char *suggestedTitle) {
  getTree()->addDragItem(DD_PEITEM, this);
  return 1;
}

ObjDirFolderItem::ObjDirFolderItem()
 : PEItem(PEI_FOLDER) {
  setIcon(new SkinBitmap("studio.icon.folder"));
}

int ObjDirFolderItem::dragOver(RootWnd *sourceWnd) {
  if (DI(sourceWnd).dragCheckData(DD_PEITEM) != -1) return 1;
  return 0;
}

int ObjDirFolderItem::dragDrop(RootWnd *sourceWnd) {
  int pos, nitems;
  if ((pos = DI(sourceWnd).dragCheckData(DD_PEITEM, &nitems)) != -1) {
    PEItem *pei = (PEItem *)DI(sourceWnd).dragGetData(pos, 0);
    ASSERT(pei != NULL);
    TreeWnd *tree = getTree();
    tree->moveTreeItem(pei, this);
    tree->expandItem(this);
    tree->ensureItemVisible(pei);
//    tree->expandItemDeferred(pei);	// open it
    tree->selectItemDeferred(pei);	// select it
    return 1;
  }
  return 0;
}

void ObjDirFolderItem::saveSettings(int &id) {
  for (int i = 0; ; i++) {
    PEItem *item = static_cast<PEItem *>(getNthChild(i));
    if (item == NULL) break;
    item->saveSettings(id);
  }
}

PlItem::PlItem(PlaylistI *pl) :
  PEItem(PEI_PLAYLIST)
{
  ASSERT(pl != NULL);
  _playlist = pl;
  last_pos = _playlist->getLastPos();
  shutdown_properly = 0;
  resetLabel();	// gets label name from playlist
  loadPl();	// mark playlist as ours
}

PlItem::PlItem(const char *_plname, const char *name, int _last_pos)
: PEItem(PEI_PLAYLIST), last_pos(_last_pos), plname(_plname)
{
  _playlist = NULL;
  shutdown_properly = 0;

  setLabel(name);
}

PlItem::~PlItem() {
_asm { int 3 };
  delete _playlist;
ASSERT(viewer_getNumItems() == 0);
  // if this is a mid-session deletion, we need to kill our disk-backed file
  if (!shutdown_properly && !plname.isempty()) {
    UNLINK(plname);
  }
}

void PlItem::onSelect() {
  // load the playlist if needed

  Editor *pledit;

  // make the playlist editor edit us
  pledit = Editor::getEditor();

  RECT src, dest;
  PlDir *pldir = PlDir::getPlDir();
  if (pldir->isVisible()) {
    pldir->getItemRect(this, &src);
    pldir->clientToScreen(&src);
    pledit->getWindowRect(&dest);
    api->drawAnimatedRects(&src, &dest);
  }

  pledit->setPlaylist(getPlaylist());
}

void PlItem::onDeselect() {
  Editor *pledit;
  pledit = Editor::getEditor();
  pledit->setPlaylist(NULL);
}

int PlItem::onContextMenu(int x, int y) {
  ContextMenu(getTree(), &DragItemT<Playlist>(getPlaylist()));
  return 1;
}

int PlItem::onChar(UINT key) {
  switch (key) {
    case VK_DELETE:
      Editor *pledit;
      pledit = Editor::getEditor();
      if (pledit->getPlaylist() == getPlaylist())
        pledit->setPlaylist(NULL);
      delete this;
      return 1;
    break;
  }
  return 0;
}

int PlItem::onBeginLabelEdit() {
//  setLabel(playlist->getName());
  return 0;
}

int PlItem::onEndLabelEdit(const char *newname) {
  if (newname == NULL || STRLEN(newname) < 1) return 0;
  getPlaylist()->setPlaylistName(newname);

  resetLabel();

  return 1;
}

int PlItem::customDraw(Canvas *canvas, const POINT &pt, int defaultTxtHeight, int indentation, const RECT &clientRect) {
  canvas->pushTextColor(plitemcolor);
  int r = PLITEM_PARENT::customDraw(canvas, pt, defaultTxtHeight, indentation, clientRect);
  canvas->popTextColor();
  return r;
}

PlaylistI *PlItem::getPlaylist() {
  if (_playlist != NULL) return _playlist;
  ASSERT(!plname.isempty());
  // load playlist if necessary
  loadPl();
  ASSERT(_playlist != NULL);
  return _playlist;
}

int PlItem::dragOver(RootWnd *sourceWnd) {
  if (DI(sourceWnd).dragCheckData(DD_FILENAME) != -1) return 1;
  return 0;
}

int PlItem::dragDrop(RootWnd *sourceWnd) {
  int ret=0;

  Playlist *playlist = getPlaylist();

  int prevnitems = playlist->getNumEntries();
  Editor *pledit = Editor::getEditor();
  bool being_edited = (pledit->getPlaylist() == playlist);
  if (being_edited) pledit->deselectAll();

  int pos, nitems;
  if ((pos = DI(sourceWnd).dragCheckData(DD_FILENAME, &nitems)) != -1) {
    for (int i = 0; i < nitems; i++) {
      Filename *fn = (Filename *)DI(sourceWnd).dragGetData(pos, i);
      const char *playstring = fn->getFilename();
      api->metadb_addNewItem(playstring);
      playlist->addPlaylistItem(playstring);
    }
    if (being_edited) pledit->resetPlaylistAdd();
    ret = nitems;
  }

  getTree()->selectItemDeferred(this);

  if (pledit->getPlaylist() == playlist)
    pledit->ensureItemVisible(prevnitems);

  return ret;
}

void PlItem::saveSettings(int &id) {
  StringPrintf tag("pldir/pl_%d", id);
  int savepos = 0;
  // if it's NULL, we never read it, so leave it alone
  if (_playlist != NULL) {
//FUCKO    if (plname.isempty()) plname = PlaylistI::inventFileName();

    _playlist->writeToFile(plname);

    savepos = _playlist->getLastPos();

    // if it's the playlist currently being edited, save that too
    if (Editor::getEditor()->getPlaylist() == _playlist)
      WASABI_API_CONFIG->setStringPrivate("pldir/last", plname);
  }
  WASABI_API_CONFIG->setStringPrivate(tag, plname);
  // save scroll pos
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("pldir/pl_scrolly_%d", id), savepos);
  // save label too
  WASABI_API_CONFIG->setStringPrivate(StringPrintf("pldir/pl_label_%d", id), getLabel());

  id++;
  shutdown_properly = 1;
}

int PlItem::viewer_onEvent(Playlist *item, int event, int param, void *ptr, int ptrlen) {
  switch (event) {
    case Playlist::Event_NAMECHANGE:
#ifdef SHOWNITEMS
    case Playlist::Event_ITEMS_ADDED:
    case Playlist::Event_ITEMS_DELETED:
    case Playlist::Event_RELOAD:
#endif
      resetLabel();
    break;
  }
  return 1;
}

void PlItem::resetLabel() {
  ASSERT(_playlist != NULL);
  const char *name = _playlist->getPlaylistName();
  if (name == NULL) return;
#ifdef SHOWNITEMS
  StringPrintf newname("%s%s (%d items)", name, _playlist->isModified() ? " *" : "", _playlist->getNumEntries());
#else
  StringPrintf newname("%s%s", name, _playlist->isModified() ? " *" : "");
#endif
  setLabel(newname);
}

void PlItem::loadPl() {
  if (_playlist == NULL) {
    ASSERT(!plname.isempty());
    // load it
    _playlist = new PlaylistI();
    int r = _playlist->readFromFile(plname);
  }
  // point back to us
//FUCKO  _playlist->setlParam(this);
  // keep track of it
  viewer_delAllItems();
  viewer_addViewItem(_playlist);
  // restore last scroll pos
  _playlist->setLastPos(last_pos);
  // set our label to match playlist name
  resetLabel();
}
