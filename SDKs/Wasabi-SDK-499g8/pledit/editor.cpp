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
#include "editori.h"

#include <bfc/std.h>

#include <studio/api.h>
#include <core/buttons.h>

#include "../common/metatags.h"
#include "../common/popup.h"
#include "../common/filename.h"
#include "../bfc/skinclr.h"
#include "../bfc/pair.h"
#include <bfc/parse/pathparse.h>
#include "../bfc/notifmsg.h"
#include "../bfc/util/timefmt.h"
#include "../bfc/svc_enum.h"

#include <core/corehandle.h>
#include "../common/script/c_script/h_text.h"
#include "../common/xlatstr.h"
#include "../common/editwndstring.h"
#include "../common/dragitemi.h"
#include "../common/selectfile.h"

#include "svc_pldir.h"

#include <bfc/attribs/attribs.h>

#include "pledit.h"
#include "playlist.h"
#include "playlisti.h"
#include "plstatus.h"

enum {
  X_SELECT, X_SHOWMODIFIED, X_SHOWLOCKED, X_REMOVEONDELETE, X_MAIN,
  X_ENQUEUEONCLICK, X_COLUMNS, X_FORCEVIRTUAL,
};

enum {
  CMD_ADD=1,
  CMD_DEL,
    CMD_DELSEL,
    CMD_DELUNSEL,
    CMD_DELDUPS,
    CMD_DELALL,
    CMD_DELDEAD,
  CMD_SEL,
    CMD_SELALL,
    CMD_DESELALL,
    CMD_SELDUPS,
    CMD_INVSEL,
    CMD_SELDISABLED,
  CMD_SORT,
  CMD_NEW,
  CMD_LISTOPT,
};

// codes for deferred callbacks
enum {
  RESETPL = 10, SETPL=20, RESETTIME=30, RESETAPPCMD=40,
};

const int STICKYBORDER = 32;

static SkinColor text("pledit.text"), rotext("pledit.text.readonly");
static SkinColor text_disabled("pledit.text.disabled");
static SkinColor bgcolor("pledit.bgcolor");
static SkinColor currenttext("pledit.text.current");
static SkinColor currentoutline("pledit.currentoutline");

#define RBORDER 2

enum {
  COLUMN_QUEUED = 1,
  COLUMN_NAME,
  COLUMN_LENGTH,
};

extern _bool show_numbers;

EditorI::EditorI() :
  playlist(NULL),
  dragInsertPos(-1),
  noItemSelDraw(FALSE),
  last_setplname(0),
  need_length_reset(0)
{
  memset(&invalidRect, 0, sizeof invalidRect);

  marker = "pledit.marker";
  dropind = "pledit.dropind";

  jumpoffset = 0;
  showmodified = TRUE;
  showlocked = TRUE;
  ismain = 0;
  islocked = 0;
  removeondelete = 0;
  enqueueonclick = 0;
  reversed_last_time = 0;

  pldir = NULL;

//XuiObject
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "select", X_SELECT);
  addParam(myxuihandle, "showmodified", X_SHOWMODIFIED);
  addParam(myxuihandle, "showlocked", X_SHOWLOCKED);
  addParam(myxuihandle, "removeondelete", X_REMOVEONDELETE);
  addParam(myxuihandle, "bidermanr0x", X_REMOVEONDELETE);
  addParam(myxuihandle, "main", X_MAIN);
  addParam(myxuihandle, "enqueueonclick", X_ENQUEUEONCLICK);
  addParam(myxuihandle, "columns", X_COLUMNS);
  addParam(myxuihandle, "forcevirtual", X_FORCEVIRTUAL);

  need_reset = 0;

  setPlaylistName();
}

EditorI::~EditorI() {
  api->service_release(pldir);
  getGuiObject()->guiobject_removeAppCmds(this);
}

void *EditorI::dependent_getInterface(const GUID *classguid) {
  HANDLEGETINTERFACE(Editor);
  return NULL;
}

void *EditorI::getInterface(GUID interface_guid) {
  if (interface_guid == Editor::getInterfaceGuid()) {
    return static_cast<Editor*>(this);
  }
  return EDITORI_PARENT::getInterface(interface_guid);
}

int EditorI::onInit() {
  EDITORI_PARENT::onInit();

  pldir = SvcEnumByGuid<svc_plDir>();
  viewer_addViewItem(pldir->getDependencyPtr());

  resetAppCmds();

  applySetColumns();

  setOwnerDraw(TRUE);

  recalcHeaders();

  viewer_addViewItem(&show_numbers);

  RootWnd *p=getDesktopParent();
  ASSERT(p != NULL);
  api->locales_registerAcceleratorSection("pledit", p);

  return 1;
}

void EditorI::onVScrollToggle(BOOL set) {
  onResize();
}

DragItemI *EditorI::itemlistwnd_getDragItem(int x, int y) {
  if (hitTest(x, y) >= 0) return NULL;

  // if none were selected contextmenu the playlist itself
  return new DragItemT<Playlist>(playlist);
}

DragItemI *EditorI::itemlistwnd_getSecondDragItem(int n) {
  if (n == 0) {
    if (playlist != NULL && !playlist->isLocked()) {
      return new DragItemT<Playlist>(playlist);
    }
  }
  return NULL;
}

void EditorI::itemlistwnd_addCustomContextMenuCommands(ContextMenu *cm) {
  if (playlist != NULL && playlist->getNumEntries()) {
    int p = 0;
    cm->addCommand("Play focused item", -20, FALSE, FALSE, p++);
    cm->addCommand("Enqueue next", -30, FALSE, FALSE, p++);
#ifdef WIN32
    cm->addCommand("Show file location", -35, FALSE, FALSE, p++);
#endif
    if (!isLocked()) cm->addCommand("Remove selected\tDel", -40, FALSE, FALSE, p++);
    if (Std::keyDown(VK_SHIFT)) cm->addCommand("Physically remove selected", -50, FALSE, FALSE, p++);
    cm->addSeparator(p++);
  }
}

void EditorI::itemlistwnd_contextMenuResult(int res) {
  switch (res) {
    case -20: fakeDoubleClick(getItemFocused()); break;	//play!
    case -30: {
      int n = playlist->getNumEntries();
      for (int i = 0; i < n; i++) {
        if (getItemSelected(i)) enqueueItem(i, TRUE);
      }
    }
    break;
#ifdef WIN32
    case -35: {
      int n = playlist->getNumEntries();
      for (int i = 0; i < n; i++) {
        if (getItemFocused(i)) {
          const char *p = playlist->enumItem(i);
          if (!STRNINCMP(p, "file:")) {
            String ps(p + STRLEN("file:"));	// kill file:
            char *p = const_cast<char *>(Std::filename(ps));
            if (p != NULL) {
              p[-1] = '\0';
              Std::shellExec(ps);
            }
          }
        }
      }
    }
    break;
#endif
    case -40: onDelete(); break;
    case -50: {
      int restart;
      do {
        restart = 0;
        int n = playlist->getNumEntries();
        for (int i = 0; i < n; i++) {
          if (getItemSelected(i)) {
            String ps = playlist->enumItem(i);
            if (FDELETE(ps, FALSE) == 1) {
              playlist->removeByPos(i);
              restart = 1;
            }
            if (restart) break;
          }
        }
      } while (restart);
    }
    break;
  }
}

int EditorI::onAcceleratorEvent(const char *name) {
  if (STRCASEEQL(name, "delete")) {
    onDelete();
  } else if (STRCASEEQL(name, "new_window")) {
    PlaylistHandle plh = playlist ? playlist->getHandle() : INVALID_PLAYLIST_HANDLE;
    createNewEditor(plh, getFirstItemVisible());
  } else if (STRCASEEQL(name, "showcurrent")) {
    if (playlist != NULL) {
      int pos = playlist->getCurrent();
      if (pos >= 0) ensureItemVisible(pos);
    }
  } else if (STRCASEEQL(name, "jumpoffset")) {
ASSERT(0);
    // find first selected item
    int sel = getFirstItemSelected();
    if (sel == -1) return 0;
    for (int f = 1; f <= jumpoffset; f++) {
      int nextsel = getNextItemSelected(sel);
      if (nextsel < 0) {
        // hit the end, wrap to beginning
        sel = getFirstItemSelected(); ASSERT(sel != -1);
        jumpoffset = 0;
        break;
      }
      sel = nextsel;
    }
    jumpoffset++;	// offset jump pos for next time
    ASSERT(sel >= 0);

    setItemFocused(sel);
  } else if (STRCASEEQL(name, "view_file_info")) {
    // {6A17D510-7833-4cec-A420-F08F4B280782}
    const GUID infoedit_guid =
    { 0x6a17d510, 0x7833, 0x4cec, { 0xa4, 0x20, 0xf0, 0x8f, 0x4b, 0x28, 0x7, 0x82 } };
    int sel = getFirstItemSelected();

    if (sel != -1) {
      const char *ps = playlist->enumItem(sel);
      if (ps != NULL) {
        api->cmd_sendCommand(infoedit_guid,"displayfile",0,0,(void *)ps,STRLEN(ps)+1);
      }
    }
  } else
    return EDITORI_PARENT::onAcceleratorEvent(name);
  return 1;
}

int EditorI::addMoreDragTypes(int pos) {
  addDragItem(DD_PLENTRY, playlist->enumEntry(pos));
  return 0;
}

int EditorI::checkDragTypes(RootWnd *sourceWnd) {
  if (DI(sourceWnd).dragCheckData(DD_PLENTRY) != -1) return 1;
  if (DI(sourceWnd).dragCheckData(DD_FILENAME) != -1) return 1;
  return EDITORI_PARENT::checkDragTypes(sourceWnd);
}

void EditorI::onExternalDropBegin() {
  getGuiObject()->guiobject_setStatusText("Scanning dropped file list... (please wait)");
}

void EditorI::onExternalDropDirScan(const char *dirname) {
  getGuiObject()->guiobject_setStatusText(StringPrintf("Scanning dir: %s", dirname));
}

void EditorI::onExternalDropEnd() {
  setPlaylistName();
}

void EditorI::onSetName() {
  EDITORI_PARENT::onSetName();
  getGuiObject()->guiobject_setStatusText(getName());
}

void EditorI::onItemSelection(int itemnum, int selected) {
  EDITORI_PARENT::onItemSelection(itemnum, selected);
  if (playlist == NULL) return;
  setPlaylistName();
  dependent_sendEvent(Editor::depend_getClassGuid(), selected ? Event_ENTRYSELECTED : Event_ENTRYDESELECTED, itemnum);
}

int EditorI::onPaint(Canvas *canvas) {
  ASSERT(canvas != NULL);
  canvas->pushTextSize(getFontSize());
  if (show_numbers)
    cwidth = canvas->getTextWidth(StringPrintf("%d.  ", getNumItems()));
  else
    cwidth = 0;
  twidth = canvas->getTextWidth("NNN:NN")+RBORDER;
  canvas->popTextSize();
  return EDITORI_PARENT::onPaint(canvas);
}

int EditorI::onLeftButtonUp(int x, int y) {
  int r = EDITORI_PARENT::onLeftButtonUp(x, y);

  return r;
}

void EditorI::onLeftClick(int itemnum) {
  if (itemnum >= 0 && getItemFocused(itemnum)) {
    enqueueItem(itemnum, enqueueonclick);
  }
  EDITORI_PARENT::onLeftClick(itemnum);
}

void EditorI::userRender(int pos, const char *playstring, Canvas &c, RECT &r, LPARAM lParam) {
  switch (lParam) {
    case COLUMN_QUEUED: {
      if (playlist == NULL) return;
      int qpos;
      if ((qpos = playlist->getQueuePos(pos))>=0) {
        int height = r.bottom - r.top;
        int cw = r.right - r.left;
        int ch = r.bottom - r.top;
        RECT p = Std::makeRect((cw - marker.getWidth())/2, (ch-marker.getHeight())/2, 0, 0);
        p.right = p.left + marker.getWidth();
        p.bottom = p.top + marker.getHeight();
        Std::offsetRect(&p, r.left, r.top);
        
        marker.stretchToRectAlpha(&c, &p, 128);
        char buf[100];
        SPRINTF(buf, "%d", qpos+1);
        c.pushTextSize(ch-1);
        c.pushTextColor(RGB(0, 0, 0));
        Std::offsetRect(&p, 1, 1);
        c.textOutCentered(&p, buf);

        Std::offsetRect(&p, -2, -2);
        c.textOutCentered(&p, buf);

        Std::offsetRect(&p, 1, 1);
        c.popTextColor();
        c.pushTextColor(RGB(255, 255, 0));
        c.textOutCentered(&p, buf);
        c.popTextColor();
        c.popTextSize();
      }
    }
    break;
    case COLUMN_NAME: {
      if (playlist == NULL) break;
      PlaylistEntry *entry = playlist->enumEntry(pos);
      COLORREF color;
      if (!entry->getCachedEnabled()) color = text_disabled;
      else if (isLocked()) color = rotext;
      else color = text;
      c.pushTextColor(color);
      if (cwidth > 0) {
        char pos_text[256];
        SPRINTF(pos_text, "%d.", pos+1);
        c.textOut(r.left, r.top, pos_text);
      }
      int cl = entry->getCachedLength();
      // italicize if no metadata
      c.pushTextItalic(entry->getRenderMode() & 1);
      c.textOutEllipsed(r.left+cwidth, r.top, r.right-r.left-(cwidth+(cl>0?twidth:0)), r.bottom-r.top, entry->getCachedName());
      c.popTextItalic();
      if(cl>0) {
        c.pushTextAlign(ALIGN_RIGHT);
        char buf[256];
        TimeFmt::printMinSec(cl/1000, buf, sizeof buf);
        c.textOut(r.right-(twidth+RBORDER), r.top, twidth, r.bottom - r.top, buf);
        c.popTextAlign();
      }
      c.popTextColor();
    }
    break;
    case COLUMN_LENGTH: {
      PlaylistEntry *entry = playlist->enumEntry(pos);
      int cl = entry->getCachedLength();
      if (cl > 0) {
        c.pushTextAlign(ALIGN_RIGHT);
        r.right--;
        api->metadb_renderData(&c, r, (void*)&cl, MDT_TIME);
        r.right++;
        c.popTextAlign();
      }
    }
    break;
  }
}

const char *EditorI::convertlParam(LPARAM lParam) {
  return playlist->enumItem(lParam);
}

void EditorI::userColumnToText(int pos, const char *playstring, LPARAM lParam, char *str, int maxlen) {
  switch (lParam) {
    case COLUMN_QUEUED: {
      if (playlist == NULL) return;
      int qpos;
      if ((qpos = playlist->getQueuePos(pos))>=0) {
        qpos++;
        STRNCPY(str, StringPrintf("%d in queue", qpos), maxlen);
      }
    }
    break;
    case COLUMN_NAME: { 
      if (playlist == NULL) break;
      PlaylistEntry *entry = playlist->enumEntry(pos);
      String s = entry->getCachedName();

      int cl = entry->getCachedLength();
      char buf[256];
      TimeFmt::printMinSec(cl/1000, buf, sizeof buf);

      if (!s.isempty()) s += ", ";
      s += buf;

      if (!entry->getCachedEnabled()) {
        if (!s.isempty()) s += ", ";
        s+="disabled";
      }
      STRNCPY(str, s, maxlen);
      break;
    }
    case COLUMN_LENGTH: {
      PlaylistEntry *entry = playlist->enumEntry(pos);
      int cl = entry->getCachedLength();
      if (cl > 0) 
        api->metadb_convertToText((void*)&cl, MDT_TIME, str, maxlen);
    }
    break;
  }
}


int EditorI::dragEnter(RootWnd *sourceWnd) {
  EDITORI_PARENT::dragEnter(sourceWnd);
  if (!checkDragTypes(sourceWnd)) return 0;
  if (playlist && playlist->isLocked()) return 0;
  dragInsertPos = -1;
  DI(sourceWnd).dragSetSticky(this, 0, 0, STICKYBORDER, STICKYBORDER);
  return 1;
}

int EditorI::dragOver(int x, int y, RootWnd *sourceWnd) {
  EDITORI_PARENT::dragOver(x, y, sourceWnd);
  if (!checkDragTypes(sourceWnd)) return 0;
  if (playlist && playlist->isLocked()) return 0;

  POINT pos={x,y};
  screenToClient(&pos); 
//  RECT r;
  int item;

  if (playlist == NULL) return 1;	// we'll add one if necessary

  item = hitTest(pos);
  if (item >= 0) {
    RECT ir, prevInvalid = invalidRect;

    dragInsertPos = item;

    getItemRect(item, &ir);

    int h = dropind.getHeight();

    int y = pos.y - ir.top;
    if (y < (ir.bottom - ir.top) / 2) {
      ir.top -= h/2;
      ir.bottom = ir.top + h;
      dragInsertPos -= 1;		// account for add-after semantics
    } else {
      ir.top = ir.bottom - h/2;
      ir.bottom = ir.top + h;
    }

    if (MEMCMP(&ir, &prevInvalid, sizeof(RECT))) {
      // erase the old marker
      invalidateRect(&invalidRect);
      repaint();

      WndCanvas c;
      c.attachToClient(this);

      RECT r2 = ir;
      if (renderRatioActive())
        multRatio(&r2);

      dropind.stretchToRectAlpha(&c, &r2);
    }

    invalidRect = ir;
  } else {
    // erase the old marker
    invalidateRect(&invalidRect);
    repaint();

    // scrolly!
    if (item == LW_HT_ABOVE) {
      //listView->scroll(-1); //FG> fucko, implement me!
    } else if (item == LW_HT_BELOW) {
      //listView->scroll(1); //FG> same here!
    }
  }

  return 1;
}

int EditorI::dragLeave(RootWnd *sourceWnd) {
  EDITORI_PARENT::dragLeave(sourceWnd);

  if (!checkDragTypes(sourceWnd)) return 0;
  if (playlist && playlist->isLocked()) return 0;

  if (playlist == NULL) return 1;

  // erase the old marker
  InvalidateRect(gethWnd(), &invalidRect, TRUE);
  memset(&invalidRect, 0, sizeof invalidRect);
  dragInsertPos = -1;

  return 1;
}

int EditorI::dragDrop(RootWnd *sourceWnd, int x, int y) {
  if (!checkDragTypes(sourceWnd)) return 0;

  // erase the old marker
  invalidateRect(&invalidRect);
  Std::setRect(&invalidRect, 0, 0, 0, 0);

  // adjust dragInsertPos if it's below or whatever
  if (playlist != NULL) {
    if (playlist->isLocked()) return 0;	// locked playlist
    RECT r;
    getClientRect(&r);
    screenToClient(&x, &y);
    int ht = hitTest(x, y);
    switch (ht) {
      case LW_HT_ABOVE: // above
        dragInsertPos = -1;
      break;
      case LW_HT_BELOW:	// below
      case LW_HT_DONTKNOW:
        dragInsertPos = playlist->getNumEntries()-1;
      break;
    }
  }

  int nitems, pos = -1, plentrypos = -1, fnpos = -1;
  pos = plentrypos = DI(sourceWnd).dragCheckData(DD_PLENTRY, &nitems);
  if (plentrypos == -1) {// prefer plentries to raw filenames
    pos = fnpos = DI(sourceWnd).dragCheckData(DD_FILENAME, &nitems);
  }

  if (pos != -1) {
    // invent playlist as necessary
    if (playlist == NULL) {
      const char *name = DI(sourceWnd).dragGetSuggestedDropTitle();
      getPlaylist(TRUE);	// force one to be created
      if (name != NULL && *name) playlist->setPlaylistName(name);
      dragInsertPos = -1;
    }

    // fetch the playlist entries

    getGuiObject()->guiobject_setStatusText(StringPrintf(_("Adding %d dropped files..."), nitems));

    // find the pos, insert it in there
    for (int i = 0; i < nitems; i++) {
      void *ptr = DI(sourceWnd).dragGetData(pos, i);
      const char *playstring = NULL;
      PlaylistEntry *entry=NULL;
      if (plentrypos != -1) {
        entry = static_cast<PlaylistEntry *>(ptr);
        playstring = entry->getPlaystring();
      } else if (fnpos != -1) {
        Filename *fn = static_cast<Filename *>(ptr);
        playstring = fn->getFilename();
      }
      if (playstring == NULL) continue;

      if (api->metadb_addNewItem(playstring)) {
        // note that we are saying insert AFTER dragInsertPos
        playlist->addPlaylistItem(playstring, dragInsertPos++);
        setSelected(dragInsertPos, TRUE);
        // and now dragInsertPos is our #
        PlaylistEntry *new_entry = playlist->enumEntry(dragInsertPos);
        if (sourceWnd == this && entry != NULL) {
          if(entry->getCurrent())
            new_entry->setCurrent(TRUE);
          // fix the queue
          playlist->replaceQueueEntry(entry, new_entry);
        }
      }
    }

    deselectAll();

    dragInsertPos = -1;

    setPlaylistName();

    return 1;
  }

  return 0;
}

// it's been dropped off!
int EditorI::dragComplete(int success) {
  int pos, nitems;
  ASSERT(playlist != NULL);	// this would be retarded
  if (!playlist->isLocked() && success && (pos = dragCheckData(DD_PLENTRY, &nitems)) != -1 && !Std::keyDown(VK_SHIFT)) {
    for (int i = 0; i < nitems; i++) {
      PlaylistEntry *entry = (PlaylistEntry *)dragGetData(pos, i);
      playlist->removeByEntry(entry);
    }
  }
  EDITORI_PARENT::dragComplete(success);
  return 1;
}

void EditorI::onItemChange(int pos, const char *playstring) {
  EDITORI_PARENT::onItemChange(pos, playstring);
  if (playlist == NULL) return;
  PlaylistEntry *entry = playlist->enumEntry(pos);
  ASSERT(entry != NULL);
  entry->delCachedInfo();
  setPlaylistName();
  dependent_sendEvent(Editor::depend_getClassGuid(), Event_REPAINT);
}

void EditorI::onItemDel(int pos, const char *playstring) {
  EDITORI_PARENT::onItemDel(pos, playstring);
  onItemChange(pos, playstring);
}

int EditorI::setPlaylist(Playlist *pl) {

  if (playlist == pl) return 1;

  if (ismain && pl) pldir->setCurrentlyOpen(pl->getHandle());

  // close out old list
  if (playlist != NULL) {
    onPlaylistUnload(playlist);
    viewer_delViewItem(playlist->getDependencyPtr());
  }

  playlist = NULL;
  deleteAllItems();
  playlist = pl;

  if (playlist != NULL) {
    // keep track of this item
    viewer_addViewItem(playlist->getDependencyPtr());

    setPlaylistName();

    // read 'em in
    resetPlaylist();

    dependent_sendEvent(Editor::depend_getClassGuid(), Event_PLAYLISTLOADED);

    // if we're the main playlist and no playlist is marked to be playing
    // then it's us
    if (ismain) {
      CoreHandle core;
      if (core.getSequencer() == NULL)
        playlist->startPlayback(FALSE);//just register w/ core
    }
  }
  postDeferredCallback(RESETAPPCMD);

  return 1;	// yay!
}

int EditorI::setPlaylistByHandle(PlaylistHandle handle, const RECT *src) {
  if (handle == INVALID_PLAYLIST_HANDLE) {
    setPlaylist(NULL);
    return 1;
  }
  Playlist *newpl = pldir->getPlaylist(handle);
if (newpl == NULL) {
  svc_objectDir *od = ObjectDirEnum("playlist").getFirst();
  newpl = static_cast<Playlist*>(od->getObject(handle));
}
  int r = setPlaylist(newpl);
  if (r && src != NULL) {
    RECT wr = windowRect();
    api->drawAnimatedRects(src, &wr);
  }
  return 1;
}

Playlist *EditorI::getPlaylist(int create_if_none) {
  ASSERT(this != NULL);
  ASSERT(pldir != NULL);
  if (playlist == NULL && create_if_none) {
    PlaylistHandle newpl = pldir->insertPlaylist();
    setPlaylist(pldir->getPlaylist(newpl));
  }
  return playlist;
}

void EditorI::setPlaylistByHandleDeferred(PlaylistHandle handle) {
  postDeferredCallback(SETPL, handle);
}

void EditorI::onPlaylistUnload(Playlist *playlist) {
  // do closing stuff
  playlist->deleteCachedInfo();
  dependent_sendEvent(Editor::depend_getClassGuid(), Event_PLAYLISTUNLOADED);
}

void EditorI::resetPlaylist() {
  setPlaylistName();
  //islocked = (playlist != NULL && playlist->getCfgItem()->getDataAsInt("Locked"));
  islocked = (playlist != NULL && playlist->isLocked());
  if (playlist == NULL) {
    deleteAllItems();
    return;
  }
  int npl = playlist->getNumEntries();
  int nlw = getNumItems();
  setRedraw(FALSE);
  noItemSelDraw=TRUE;
  if (npl > nlw) {
    setMinimumSize(npl);
    for (int i = nlw; i < npl; i++) addItem(NULL, i);
  } else if (npl < nlw) {
    while (deleteByPos(npl));
  }
  noItemSelDraw=FALSE;
  setRedraw(TRUE);
}

#if 0
void Editor::resetPlaylistAdd(int autoselect) {
  setPlaylistName();
  if (playlist == NULL) {
    deleteAllItems();
    return;
  }

  // if listview is empty, just dump it all in
  if (getNumItems() <= 0) {
    int nitems = playlist->getNumEntries();
    setRedraw(FALSE);
    setMinimumSize(nitems+256);
    for (int i = 0; i < nitems; i++) {
      PlaylistEntry *entry = playlist->enumEntry(i);
      ASSERT(entry != NULL);
      addItem(NULL, (LPARAM)entry);
    }
    if (autoselect) selectAll();
    setRedraw(TRUE);
    return;
  }
  // account for empty playlist
  if (playlist->getNumEntries() <= 0) return;
  // comparison loop
  int playlistpos = 0;
  int listviewpos = 0;

  setRedraw(FALSE);

  int firstscrolled=0;

  for (;; playlistpos++, listviewpos++) {
    PlaylistEntry *plentry = playlist->enumEntry(playlistpos);
    if (plentry == NULL) break;	// we're done
    PlaylistEntry *lventry;
    if (getNumItems() > listviewpos)
      lventry = (PlaylistEntry *)getItemData(listviewpos);
    else
      lventry = NULL;
    
    // end of listview?
    if (lventry == NULL) {
      // dump the rest in
      int nit = playlist->getNumEntries();
      for (int i = playlistpos; i < nit; i++) {
        int p = addItem(NULL, (LPARAM)playlist->enumEntry(i));
        if (autoselect) {
          setSelected(p, TRUE);
        }
        if (!firstscrolled) ensureItemVisible(getNumItems()-1);
        firstscrolled=1;
      }
      break;	// we're done
    }

    // equal, leave alone
    if (lventry == plentry)
      continue;

    // insert needed item
    int p = insertItem(listviewpos, NULL, (LPARAM)playlist->enumEntry(playlistpos));
    if (autoselect) {
      setSelected(p, TRUE);
    }
    if (!firstscrolled) ensureItemVisible(listviewpos);
    firstscrolled=1;
    // we just forced the pointers to be equal, so allow to advance
  }

  setRedraw(TRUE);
}

void Editor::resetPlaylistDel() {
  setPlaylistName();
  if (playlist == NULL) {
    deleteAllItems();
    return;
  }

  setRedraw(FALSE);

  int playlistpos = 0;
  int listviewpos = 0;
  int firstemptypos = -1;
  for (;; playlistpos++, listviewpos++) {
    PlaylistEntry *plentry = playlist->enumEntry(playlistpos);
    if (plentry == NULL) {
      // delete until end of listview
      int lvnitems = getNumItems();
      for (int i = playlistpos; i < lvnitems; i++)
        deleteByPos(listviewpos);
      break;	// out of compare loop
    }
    PlaylistEntry *lventry;
    if (getNumItems() > listviewpos)
      lventry = (PlaylistEntry *)getItemData(listviewpos);
    else
      lventry = NULL;

    ASSERT(lventry != NULL); 

    // equal, leave alone
    if (lventry == plentry)
      continue;

    // Save the position of the first item that we delete, to allow us to be
    // able to just hold down the delete key and see the proper functionality
    // (delete down until the selection hits the end, then delete up to top)
    if (firstemptypos == -1) {
      firstemptypos = listviewpos;
    }

    deleteByPos(listviewpos);
    playlistpos--;
    listviewpos--;
  }

  // After we're all done, make sure we regenerate proper state info.  Bugs
  // happen if we don't explicitly define who to look at and who is selected.
  if (getNumItems() >= 1) {
    if (firstemptypos == -1) {
      firstemptypos = getNumItems() - 1;
    } else {
      firstemptypos = MIN(firstemptypos, getNumItems() - 1);
    }
    ensureItemVisible(firstemptypos);
    setSelectionStart(firstemptypos);
  }

  setRedraw(TRUE);
}
#endif

int EditorI::fakeDoubleClick(int pos) {
  if (!playlist) return 0;
  if (pos < 0 || pos >= playlist->getNumEntries()) return 0;
  onDoubleClick(pos);
  return 0;
}

void EditorI::deleteSelected() {
  onDelete();
}

typedef Pair<PlaylistEntry*, int> pair;
struct ComparePair {
  static int compareItem(pair *p1, pair *p2) { return CMP3(p1->a->getPlaystring(), p2->a->getPlaystring()); }
};

int EditorI::selectDups() {
  deselectAll();
  if (playlist == NULL) return 0;

  int dups_found = 0;

  int nentries = playlist->getNumEntries();
  PtrListQuickSorted<pair, ComparePair> copy(nentries);

  int i;
  for (i = 0; i < nentries; i++) {
    pair *p = new pair(playlist->enumEntry(i), i);
    copy.addItem(p);
  }
  noItemSelDraw=TRUE;
  const char *last = NULL;
  int select_first=1;
  for (i = 0; i < nentries; i++) {
    pair *pair = copy[i];
    const char *tp = pair->a->getPlaystring();
    if (last != NULL && STREQL(tp, last)) {
      setSelected(pair->b, TRUE); dups_found++;
      if (select_first) { setSelected(copy[i-1]->b, TRUE); dups_found++; }
      select_first = 0;
    } else {
      select_first = 1;
    }
    last = tp;
  }
  noItemSelDraw=FALSE;
  setPlaylistName();

  copy.deleteAll();

  return dups_found;
}

int EditorI::selectDisabled() {
  deselectAll();
  if (playlist == NULL) return 0;
  noItemSelDraw=TRUE;
  int nentries = playlist->getNumEntries();
  int nfound = 0;
  for (int i = 0; i < nentries; i++) {
    PlaylistEntry *entry = playlist->enumEntry(i);
    if (!entry->getCachedEnabled()) {
      setSelected(i, TRUE);
      nfound ++;
    }
  }
  noItemSelDraw=FALSE;
  setPlaylistName();

  return nfound;
}

void EditorI::setJumpText(const char *txt) {
  if (jumptext.iscaseequal(txt)) return;
  jumptext = txt;
  jumptext.tolower();
  if (txt == NULL || *txt == 0) {
    deselectAll();
    return;
  }

  // break search text into individual words
  PathParser pp(jumptext, " ");
  if (pp.getNumStrings() <= 0) {
    deselectAll();
    return;
  }
  int nkeys = pp.getNumStrings();

  int sav = setRedraw(FALSE);
  if (playlist == NULL) return;
  int n = playlist->getNumEntries();
  char buf[1024];
  for (int i = 0; i < n; i++) {
    PlaylistEntry *entry = playlist->enumEntry(i);
    const char *name = entry->getCachedName();
    int sel = 1;
    STRNCPY(buf, name, 1023);
    buf[1023] = '\0';
    STRTOLOWER(buf);
    for (int j = 0; j < nkeys; j++) {
      if (STRSTR(buf, pp.enumString(j)) == NULL) {
        sel = 0;
        break;
      }
    }
    setSelected(i, sel);
  }
  setRedraw(sav);
}

COLORREF EditorI::getTextColor(LPARAM lParam) {
  if (playlist == NULL) return text;
  PlaylistEntry *entry = playlist->enumEntry(lParam);
  if (entry && entry->getCurrent())
    return currenttext;
  else
    return text;
}

COLORREF EditorI::getBgColor(LPARAM lParam) {
  return bgcolor;
#if 0
#if 0
  return bgcolor;
#else
  const int maxi = 64;
  if (entry->getCurrent()) return RGB(maxi, maxi, maxi);
  int cur_pos = playlist->getCurrent();
  if (cur_pos < 0) return bgcolor;
  for (int i = 0; i < nentries; i++) {
    if (playlist->enumEntry(i) == entry) break;
  }
  ASSERT(i < nentries);
  int intens;
  if (i < cur_pos) {
    intens = (i - cur_pos) * maxi / cur_pos - (255-maxi);
  } else  {
    intens = maxi - (i - cur_pos) * maxi / (nentries - cur_pos);
  }
  return RGB(intens, intens, intens);
#endif
#endif
}

int EditorI::getSelected(LPARAM lParam) {
  PlaylistEntry *entry = playlist->enumEntry(lParam);
  return entry->getCurrent();
}

// those functions are here for speeding up the selection stuff of listwnd
// (no status update at each selection toggle)
int EditorI::selectAll() {
  noItemSelDraw = TRUE;
  int ret=EDITORI_PARENT::selectAll();
  noItemSelDraw = FALSE;
  setPlaylistName();
  return ret;
}

int EditorI::deselectAll() {
  bool b = noItemSelDraw;
  noItemSelDraw = TRUE;
  int ret=EDITORI_PARENT::deselectAll();
  noItemSelDraw = b;
  setPlaylistName();
  return ret;
}

int EditorI::invertSelection() {
  noItemSelDraw = TRUE;
  int ret=EDITORI_PARENT::invertSelection();
  noItemSelDraw = FALSE;
  setPlaylistName();
  return ret;
}

void EditorI::setSelectionEnd(int pos) {
  noItemSelDraw = TRUE;
  EDITORI_PARENT::setSelectionEnd(pos);
  noItemSelDraw = FALSE;
  setPlaylistName();
}

void EditorI::appcmds_onCommand(int id, const RECT *buttonRect, int which_btn) {
  switch (id) {
    case CMD_ADD: {
      SelectFile sf(this, "Add ", "...");
      sf.setPopPosition(buttonRect->left, buttonRect->bottom);
      sf.runSelector(NULL, TRUE, api->core_getExtSupportedExtensions());
      if (!sf.getNumFiles()) return;
      Playlist *pl = getPlaylist(TRUE);
      extern _bool incpl;
      int rem_playlists = !incpl && STRCASEEQL(sf.getType(), "directory");
      int nfiles = sf.getNumFiles();
      for (int i = 0; i < nfiles; i++) {
        const char *fn = sf.enumFilename(i);
        if (rem_playlists && STREQLSAFE(api->core_getExtensionFamily(Std::extension(fn)), "Playlist"))
          continue;
        api->metadb_addNewItem(fn);
        pl->addPlaylistItem(fn);
      }
    }
    break;
    case CMD_DEL: {
      PopupMenu pop(this);
      pop.addCommand("Delete selected items", CMD_DELSEL);
      pop.addCommand("Delete unselected items (crop)", CMD_DELUNSEL);
      pop.addCommand("Delete duplicated items", CMD_DELDUPS);
      pop.addCommand("Delete nonexistent files", CMD_DELDEAD);
      pop.addCommand("Delete all items (clear playlist)", CMD_DELALL);
      switch (pop.popAtXY(buttonRect->left, buttonRect->bottom)) {
        case CMD_DELSEL:
          deleteSelected();
        break;
        case CMD_DELUNSEL:
          invertSelection();
          deleteSelected();
          selectAll();
        break;
        case CMD_DELDUPS:
          if (playlist != NULL) playlist->removeDups();
        break;
        case CMD_DELDEAD: {
          if (playlist != NULL) {
            int killfromdbtoo = Std::keyDown(VK_SHIFT);
            int nentries = playlist->getNumEntries();
            for (int i = 0; i < nentries; i++) {
              const char *ps = playlist->enumItem(i);
              if (!STRNINCMP(ps, "file:")) {
                if (!FEXISTS(ps+5)) {
                  String sps(ps);
                  playlist->removeByPos(i);
                  i--;
                  nentries--;
                  if (killfromdbtoo) api->metadb_deleteItem(sps);
                }
              }
            }
          }
        }
        break;
        case CMD_DELALL:
          if (playlist != NULL) playlist->deleteAll();
        break;
      }
    }
    break;
    case CMD_SEL: {
      PopupMenu pop(this);
      pop.addCommand("Select all\tCtrl-A", CMD_SELALL);
      pop.addCommand("Deselect all", CMD_DESELALL);
      pop.addCommand("Invert selection", CMD_INVSEL);
      pop.addCommand("Select duplicates", CMD_SELDUPS);
      pop.addCommand("Select disabled", CMD_SELDISABLED);
      switch (pop.popAtXY(buttonRect->left, buttonRect->bottom)) {
        case CMD_SELALL:
          selectAll();
        break;
        case CMD_DESELALL:
          deselectAll();
        break;
        case CMD_INVSEL:
          invertSelection();
        break;
        case CMD_SELDUPS: {
          int ndups = selectDups();
          StringPrintf no("%d entries were selected", ndups);
          getGuiObject()->guiobject_setStatusText(no, TRUE);
        }
        break;
        case CMD_SELDISABLED: {
          int ndis = selectDisabled();
          StringPrintf no("%d entries were selected", ndis);
          getGuiObject()->guiobject_setStatusText(no, TRUE);
        }
        break;
      }
    }
    break;
    case CMD_SORT: {
      ContextMenu(this, &DragItemT<Playlist>(getPlaylist()), FALSE, "sort").popAtXY(buttonRect->left, buttonRect->bottom);
    }
    break;
    case CMD_LISTOPT: {
      DragItemT<Playlist> di(getPlaylist());
      ContextMenu cm(this, &di, FALSE, "playlist");
      cm.addSeparator();
      cm.addCommand("Create new playlist", -10);
      cm.addCommand("Create new playlist in new editor", -20);
      int r = cm.popAtXY(buttonRect->left, buttonRect->bottom);
      switch (r) {
        case -10: {
          setPlaylistByHandleDeferred(pldir->insertPlaylist());
        }
        break;
        case -20: {
          createNewEditor(pldir->insertPlaylist());
        }
        break;
      }
    }
    break;
    default:
      AppCmdsI::appcmds_onCommand(id, buttonRect, which_btn);
  }
}

int EditorI::viewer_onItemDeleted(Dependent *item) {
  if (playlist != NULL && item == playlist->getDependencyPtr()) {
    playlist = NULL;
    deleteAllItems();
    setPlaylistName();
    postDeferredCallback(SETPL, INVALID_PLAYLIST_HANDLE);
    return 1;
  }
  return 0;
}

int EditorI::viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param2, void *ptr, int ptrlen) {
  if (pldir != NULL && item == pldir->getDependencyPtr()) {
    switch (event) {
      case svc_plDir::Event_PLAYLIST_ADDED:
        // if we're empty, edit first seen playlist
        if (playlist == NULL && !pldir->getHidden(param2)) setPlaylistByHandleDeferred(param2);
        return 1;
    }
    return 0;
  }
  if (playlist != NULL && item == playlist->getDependencyPtr()) {
    if (*classguid == *Playlist::depend_getClassGuid()) {
      switch (event) {
//CUT        case Playlist::Event_LOCK:
//CUT          if (islocked != param2) resetAppCmds();
//CUT          islocked = param2;
//CUT        // fall through
        case Playlist::Event_NAMECHANGE:
          setPlaylistName();
        break;
        case Playlist::Event_REPAINT:
          if (param2 >= 0)
            invalidateItem(param2);
          else
            invalidate();
        break;
        case Playlist::Event_ITEMS_ADDED:
        case Playlist::Event_ITEMS_DELETED:
        case Playlist::Event_RELOAD:
          if (!need_reset) {
            need_reset = 1;
            postDeferredCallback(RESETPL);
          }
        break;
        case Playlist::Event_SETCURRENT:
          ensureItemVisible(param2);
        break;
        case Playlist::Event_QUEUE_ADDITION:
          if (getColumnPosByName("Q") < 0)
            insertColumn(new ItemListColumn_Callback(this, COLUMN_QUEUED, "Q"), 16, getNumColumns());
        break;
        case Playlist::Event_QUEUE_EMPTY:
          delColumnByPos(getColumnPosByName("Q"));
        break;
        case Playlist::Event_ENSUREVISIBLE:
          ensureItemVisible(param2);
        break;
      }//switch
    } else if (*classguid == *CfgItem::depend_getClassGuid()) {
      switch (event) {
        case CfgItem::Event_ATTRIBUTE_CHANGED:
          if (STRCASEEQL((char*)ptr, "Locked") || STRCASEEQL((char*)ptr, "UserLocked")) {
            int waslocked = islocked;
//            CfgItem *ci = dynamic_guid_cast<CfgItem>(item);
//            CfgItem *ci = (CfgItem*)item->dependent_getInterface(CfgItem::depend_getClassGuid());
//ASSERT(ci != NULL);
//            islocked = ci->getDataAsInt("Locked");
          //int islocked = playlist->getCfgItem()->getDataAsInt("Locked");
islocked = playlist->isLocked();
//            if (waslocked != islocked) {
              resetAppCmds();
              setPlaylistName();
              invalidate();	// so we see the readonly color
//            }
          }
        break;
      }
    }//CfgItem event space
    return 1;
  }//if ==playlist

  if (item == &show_numbers) {
    invalidate();
    return 1;
  }
  return 0;
}

void EditorI::metacb_onItemDel(const char *playstring) {
  EDITORI_PARENT::metacb_onItemDel(playstring);
  if (removeondelete && playlist != NULL) {
    int n = playlist->getNumEntries();
    for (int i = 0; i < n; i++) {
      const char *ps = playlist->enumItem(i);
      if (STREQL(playstring, ps)) {
        playlist->removeByPos(i);
        i--;
        n--;
      }
    }
  }
}

void EditorI::setPlaylistName() {
  if (!need_length_reset) {
    postDeferredCallback(RESETTIME, 0, 400);
    need_length_reset = 1;
  }
  last_setplname = Std::getTimeStampMS();
}

void EditorI::_setPlaylistName() {
  if (playlist == NULL) {
    setName("[empty playlist]");
  } else {
//__asm int 3;
    unsigned int total_len = 0, sel_len = 0, tot_unks = 0, sel_unks = 0;
    int nentries = playlist->getNumEntries();
    for (int i = 0; i < nentries; i++) {
      PlaylistEntry *ent = playlist->enumEntry(i);
      int l = playlist->getCachedLengthForEntry(ent);
      int is_sel = getItemSelected(i);
      if (l == -1) {
        if (is_sel) sel_unks ++;
        tot_unks ++;
      }
      else {
        if (is_sel) sel_len += l / 1000;
        total_len += l/ 1000;
      }
    }

    char sellenstr[1024];
    TimeFmt::printMinSec(sel_len, sellenstr, 1024);

    char lenstr[1024];
    TimeFmt::printMinSec(total_len, lenstr, 1024);

    StringPrintf buf("%s: %d item%s (%s%s, %s%s)",
      playlist->getPlaylistName(),
      nentries, (nentries == 1) ? "" : "s",
      sellenstr,
      sel_unks ? "+" : "",
      lenstr,
      tot_unks ? "+" : "");
    if (showmodified && playlist->isModified()) buf.cat(" (modified)");
    if (showlocked && isLocked()) buf.cat(" (locked)");

    setName(buf);
  }
}

void EditorI::onDelete() {
  if (playlist == NULL) return;
  if (playlist->isLocked()) return;	// locked playlist
  ASSERT(getNumItems() == playlist->getNumEntries());

  PtrList<PlaylistEntry> list;

  int cur_pos = playlist->getCurrent();
  for (int i = 0; i < getNumItems(); i++) {
    if (getItemSelected(i)) {
      list.addItem(playlist->enumEntry(i));
      if (cur_pos == i)
        cur_pos = i+1;
    }
  }

  if (list.getNumItems() == 0) return;	// nothing was selected

  deselectAll();

  foreach(list)
    playlist->removeByEntry(list.getfor());
  endfor

  if (cur_pos >= 0) {
    if (!playlist->getNumQueued()) enqueueItem(cur_pos-list.getNumItems(), TRUE);
    playlist->setCurrent(cur_pos-list.getNumItems());
  }
}

void EditorI::onDoubleClick(int itemnum) {

  if (playlist == NULL) return;

  if (itemnum < 0 || itemnum >= playlist->getNumEntries()) return;

  playlist->setCurrent(itemnum);
  playlist->setDontRandomizeFirst();
  // if it's 0, it's #1, and it's silly to make it play twice imo
  if (playlist->getQueuePos(itemnum) != 0)
    playlist->setDontGetNextFromQueue();
  playlist->startPlayback();

  invalidate();
}

int EditorI::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return EDITORI_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);
  switch (xmlattributeid) {
    case X_SELECT:
      setJumpText(value);
    break;
    case X_SHOWMODIFIED: {
      int v = !!ATOI(value);
      if (showmodified != v) {
        showmodified = ATOI(value);
        setPlaylistName();
      }
    }
    break;
    case X_SHOWLOCKED: {
      int v = !!ATOI(value);
      if (showlocked != v) {
        showlocked = ATOI(value);
        setPlaylistName();
      }
    }
    break;
    case X_REMOVEONDELETE: {
      removeondelete = ATOI(value);
    }
    break;
    case X_MAIN: {	// wolverine r0x0
      ismain = ATOI(value);
    }
    break;
    case X_ENQUEUEONCLICK: {
      enqueueonclick = ATOI(value);
    }
    break;
    case X_COLUMNS: {
      setColumns(value);
    }
    break;
    case X_FORCEVIRTUAL: {
      setVirtual(ATOI(value));
    }
    break;
  }
  return 0;
}

int EditorI::isLocked() {
  return islocked;
}

int EditorI::onDeferredCallback(int p1, int p2) {
  switch (p1) {
    case RESETPL: {
      resetPlaylist();
      need_reset = 0;
    }
    break;
    case SETPL:
      if (p2 == INVALID_PLAYLIST_HANDLE) {
        p2 = pldir->enumPlaylist(0);
        ASSERT(p2 != NULL);
      }
      setPlaylistByHandle(p2);
    break;
    case RESETTIME: {
      stdtimevalms diff = Std::getTimeStampMS() - last_setplname;
      if (diff < .300f) {
        postDeferredCallback(RESETTIME, 0, 400); // post it again
      } else {
        _setPlaylistName();
        need_length_reset = 0;
      }
    }
    break;
    case RESETAPPCMD:
      resetAppCmds();
    break;
    default: return EDITORI_PARENT::onDeferredCallback(p1, p2);
  }
  return 1;
}

int EditorI::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source) {
  if (STRCASEEQLSAFE(action, "SENDTOPL")) {
    PopupMenu menu;
    menu.addCommand("Create new playlist", -10);
    int n = pldir->getNumPlaylists();
    if (n > 0) menu.addSeparator();
    for (int i = 0; i < n; i++) {
      PlaylistHandle hand = pldir->enumPlaylist(i);
      const char *label = pldir->getPlaylistLabel(hand);
      if (label == NULL) label = "NULLLLLLLLL";
      menu.addCommand(label, hand);
    }
    if (source != NULL) {
      RECT wr;
      source->getWindowRect(&wr);
      x = wr.left;
      y = wr.bottom;
    }
    int r = menu.popAtXY(x, y);
    PlaylistHandle hand = INVALID_PLAYLIST_HANDLE;
    Playlist *pl = NULL;
    RootWnd *newed = NULL;
    if (r == -10) {
      hand = pldir->insertPlaylist();// create it
      createNewEditor(hand, -1);
    } else hand = r;
    pl = pldir->getPlaylist(hand);
    if (pl != NULL && playlist != NULL) {
      int n = playlist->getNumEntries();
      for (int i = 0; i < n; i++) {
        const char *ps = playlist->enumItem(i);
        pl->addPlaylistItem(ps);
        if (i == 0) pl->ensureVisible(n);
      }
    }
    return 1;
  }
  if (STRCASEEQLSAFE(action, "SENDSELECTEDTOEDITOR")) {
    if (param != NULL && playlist != NULL) {
      // get the id from the param
      RootWnd *red = source->findWindow(param);
      if (red != NULL) {
        Editor *ed = static_cast<Editor*>(red->getInterface(Editor::getInterfaceGuid()));
        if (ed != NULL) {
          Playlist *pl = ed->getPlaylist();
          ASSERT(pl != NULL);
          int n = playlist->getNumEntries();
          for (int i = 0; i < n; i++) {
            if (getItemSelected(i)) {
              pl->addPlaylistItem(playlist->enumItem(i));
            }
          }
          return 1;
        }
      }
    }
  }

  return EDITORI_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, source);
}

void EditorI::enqueueItem(int itemnum, int force_key) {
  int altdown = Std::keyDown(VK_MENU) || force_key;
  int ctrldown = Std::keyDown(VK_CONTROL);
  if (altdown) {
    if (ctrldown) {
      // stick it in the front
      playlist->markNextFile(itemnum, TRUE);
    } else if (playlist->getQueuePos(itemnum) >= 0)
      // if it was on, toggle it off
      playlist->unmarkFile(itemnum);
    else {
      // enqueue it at the end
      playlist->markNextFile(itemnum, FALSE);
    }
  }
  return;
}

void EditorI::setColumns(const char *val) {
  columns_list = val;
  applySetColumns();
}

void EditorI::applySetColumns() {
  if (!isInited()) return;
  deleteAllColumns();

  extern _string def_columns;
  if (columns_list.isempty()) columns_list = def_columns;

  PathParser pp(columns_list, ";");
  if (columns_list.isempty() || pp.getNumStrings() <= 0) {
    ItemListColumn *ilc = new ItemListColumn_Callback(this, COLUMN_NAME, MT_NAME);
    ilc->setDynamic(1);
    insertColumn(ilc, -1);
  } else {
    for (int i = 0; i < pp.getNumStrings(); i++) {
      PathParser widtho(pp.enumString(i), ",");
      PathParser labelo(widtho.enumString(0), "=");
      const char *label = labelo.enumString(labelo.getNumStrings() > 1 ? 1 : 0);
      int width = widtho.getNumStrings() > 1 ? ATOI(widtho.enumString(1)) : 100;
      int dyn = (width == -1);
      int center = widtho.getNumStrings() > 2 ? ATOI(widtho.enumString(2)) : 0;
      const char *nn = labelo.enumString(0);
      ItemListColumn *col = NULL;
      if (*nn == '.') {
        if (!STRCMP(nn+1, "#")) {
          col = new ItemListColumn_Numbered;
        } else if (!STRCMP(nn+1, "enabled")) {
          col = new ItemListColumn_Enabled;
        }
      } else {
        col = new ItemListColumn_MetaTagSort(nn, center, label);
      }
      if (col != NULL) {
        col->setDynamic(dyn);
        insertColumn(col, width);
      }
    }
  }
  showColumnsHeaders = (getNumColumns() != 1);
}

void EditorI::createNewEditor(PlaylistHandle hand, int showpos) {
  RootWnd *sw = api->skinwnd_createByGroupId("pledit.main.group");
  if (sw != NULL && hand != INVALID_PLAYLIST_HANDLE) {
    RootWnd *rw = sw->findWindowByInterface(Editor::getInterfaceGuid());
    if (rw != NULL) {
      Editor *ed = static_cast<Editor*>(rw->getInterface(Editor::getInterfaceGuid()));
      if (ed != NULL) {
        ed->setPlaylistByHandle(hand);
        if (showpos >= 0) ed->ensureVisible(showpos);
      }
    }
  }
}

void EditorI::resetAppCmds() {
  appcmds_deleteAll();
  if (playlist == NULL || !playlist->isLocked()) {
    appcmds_addCmd("Add", CMD_ADD, SIDE_LEFT);
    appcmds_addCmd("Del", CMD_DEL, SIDE_LEFT);
  }
  appcmds_addCmd("Sel", CMD_SEL, SIDE_LEFT);
  appcmds_addCmd("Sort", CMD_SORT, SIDE_LEFT);
  int l = 0;
  if (playlist != NULL) {
    CfgItem *ci = playlist->getCfgItem();
    if (ci != NULL) l = ci->getDataAsInt("Locked");
  }
  if (playlist == NULL || !l) {
    appcmds_addCmd("Playlist", CMD_LISTOPT, SIDE_RIGHT);
  }

  getGuiObject()->guiobject_addAppCmds(this);
}

#define CBCLASS EditorN
START_DISPATCH;
  CB(GETDEPPTR, getDependencyPtr);
  CB(SETPLAYLISTBYHANDLE, setPlaylistByHandle);
  CB(GETPLAYLIST, _getPlaylist);
  VCB(SCROLLTOY, _scrollToY);
  CB(GETSCROLLY, _getScrollY);
  CB(ITEMSELECTED, _itemSelected);
  VCB(ENSUREVISIBLE, _ensureVisible);
  VCB(ENQUEUEITEM, _enqueueItem);
END_DISPATCH;
#undef CBCLASS

ItemListColumn_MetaTagSort::ItemListColumn_MetaTagSort(const char *tag, int center, const char *label)
  : ItemListColumn_MetaTag(tag, center, label), reverse(0), show_arrow(0),
    down_arrow("wasabi.button.label.arrow.down"),
    up_arrow("wasabi.button.label.arrow.up")
{ }

int ItemListColumn_MetaTagSort::onHeaderClick() {
  EditorI *ed = static_cast<EditorI *>(getList());
  ASSERT(ed != NULL);
  Playlist *pl = ed->getPlaylist();
  if (pl == NULL) return 1;

  pl->sortBy(getTag());
  if (reverse) pl->reverse();
  reverse = !reverse;
  show_arrow = 1;

  return 1;
}

int ItemListColumn_MetaTagSort::customDrawHeader(Canvas *c, RECT *cr) {
  // adds the little arrow
  int r = ITEMLISTCOLUMN_METATAGSORT_PARENT::customDrawHeader(c, cr);
  if (show_arrow) {
    AutoSkinBitmap &bitmap = reverse ? down_arrow : up_arrow;
    int u = ((cr->bottom - cr->top) - bitmap.getHeight())/2;
    bitmap.blitAlpha(c, cr->right - bitmap.getWidth(), cr->top+u);
  }
  return r;
}

ItemListColumn_Enabled::ItemListColumn_Enabled() :
  nextval(FALSE),
  checked("wasabi.button.checkbox.pressed"),
  unchecked("wasabi.button.checkbox")
{ }

void ItemListColumn_Enabled::render(int pos, const char *playstring, Canvas &c, RECT &r) {
  EditorI *ed = static_cast<EditorI *>(getList());
  ASSERT(ed != NULL);
  Playlist *pl = ed->getPlaylist();
  if (pl == NULL) return;
  PlaylistEntry *entry = pl->enumEntry(pos);
  if (entry == NULL) return;
  int v = pl->entryEnabled(entry);
  AutoSkinBitmap &bitmap = v ? checked : unchecked;
  int ho = ((r.right - r.left) - bitmap.getWidth())/2;
  int vo = ((r.bottom - r.top) - bitmap.getHeight())/2;
  bitmap.blitAlpha(&c, r.left+ho, r.top+vo);
}

int ItemListColumn_Enabled::onHeaderClick() {
  EditorI *ed = static_cast<EditorI *>(getList());
  ASSERT(ed != NULL);
  Playlist *pl = ed->getPlaylist();
  if (pl == NULL) return 1;
  int n = pl->getNumEntries();
  for (int i = 0; i < n; i++) {
    pl->entryEnable(pl->enumEntry(i), nextval);
  }
  nextval = !nextval;
  return 1;
}

int ItemListColumn_Enabled::onColumnLeftClick(int pos) {
  EditorI *ed = static_cast<EditorI *>(getList());
  ASSERT(ed != NULL);
  Playlist *pl = ed->getPlaylist();
  if (pl == NULL) return 1;

  PlaylistEntry *ent = pl->enumEntry(pos);
  if (ent != NULL) {
    pl->entryEnable(ent, !pl->entryEnabled(ent));
  }

  return 1;
}

int ItemListColumn_Enabled::customDrawHeader(Canvas *c, RECT *cr) {
  int h = ((cr->right - cr->left) - checked.getWidth())/2;
  int v = ((cr->bottom - cr->top) - checked.getHeight())/2;
  checked.blitAlpha(c, cr->left+h, cr->top+v);
  return 1;
}

void ItemListColumn_Enabled::columnToText(int pos, const char *playstring, char *str, int maxlen) {
  EditorI *ed = static_cast<EditorI *>(getList());
  ASSERT(ed != NULL);
  Playlist *pl = ed->getPlaylist();
  if (pl == NULL) return;
  PlaylistEntry *entry = pl->enumEntry(pos);
  if (entry == NULL) return;
  int v = pl->entryEnabled(entry);
  STRNCPY(str, v ? _("checked") : _("not checked"), maxlen);
}
