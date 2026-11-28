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
#include "plstatus.h"

#include "../bfc/ptrlist.h"
#include "../bfc/canvas.h"
#include "../bfc/skinclr.h"
#include "../studio/api.h"
#include "../bfc/util/timefmt.h"
#include "editor.h"
#include "playlisti.h"
#include "pledit.h"

// {07917C36-A49A-40e3-85DF-AA1817DA10DA}
const GUID PlStatusGuid = 
{ 0x7917c36, 0xa49a, 0x40e3, { 0x85, 0xdf, 0xaa, 0x18, 0x17, 0xda, 0x10, 0xda } };

const char * const NOFILE="[no file]";

static SkinColor skin_bkcolor("pledit.bgcolor");
static SkinColor skin_fgcolor("pledit.text");

PlStatus::PlStatus() {
  fgcolor_set = 0;
  bgcolor_set = 0;

  font = WASABI_DEFAULT_FONTNAME;
  fontsize = 14;
  antialias = 0;
  bold = 0;
  offsetx = 2;
  offsety = 2;
  title = NOFILE;
  prevpl = NULL;
//CUT  plseq = NULL;

//XuiObject
  myxuihandle = newXuiHandle();
//  addParam(myxuihandle, "font", X_SELECT);
}

PlStatus::~PlStatus() {
  api->core_delCallback(0,this);
}

int PlStatus::onInit() {
  PLEDITSTATUS_PARENT::onInit();
//CUT  Editor *editor = Editor::getEditor();
//CUT#pragma CHAT("bas", "bas", "need a findObject here!")
//CUT  if (!editor) return 0;
//CUT  initViewer(editor);
//CUT  plseq = WACpledit::getPlaylistSequencer();
  api->core_addCallback(0, this);
  return 1;
}

#if 0
void PlStatus::initViewer(Editor *e) {
  if (le_editor != NULL) return;
  le_editor = e;
  ASSERT(e != NULL);
  viewer_addViewItem(e);
  Playlist *pl = e->getPlaylist();
  if (pl != NULL) {
    viewer_addViewItem(pl->getDependencyPtr());
    setText(pl);
  }
  editor_inited = 1;
}
#endif

GUID PlStatus::getWindowTypeGuid() {
  return PlStatusGuid;
}

int PlStatus::setXmlParam(const char *param, const char *value) {
  PLEDITSTATUS_PARENT::setXmlParam(param, value);
  if (STRCASEEQL(param, "font")) { setFont(value); return 1; }
  if (STRCASEEQL(param, "bkcolor")) {
    setBkColor(api->parse(value, "color"));
    bgcolor_set = 1;
    return 1;
  }
  if (STRCASEEQL(param, "color")) {
    setColor(api->parse(value, "color"));
    fgcolor_set = 1;
    return 1;
  }
  if (STRCASEEQL(param, "fontsize")) { setFontSize(ATOI(value)); return 1; }
  if (STRCASEEQL(param, "antialias")) { setAntialias(ATOI(value)); return 1; }
  if (STRCASEEQL(param, "bold")) { setBold(ATOI(value)); return 1; }
  if (STRCASEEQL(param, "offsetx")) { setOffsetX(ATOI(value)); return 1; }
  if (STRCASEEQL(param, "offsety")) { setOffsetY(ATOI(value)); return 1; }
  return 0;
}

void PlStatus::setBkColor(COLORREF c) {
  if (bkcolor == c) return;
  bkcolor = c;
  invalidate();
}

void PlStatus::setColor(COLORREF c) {
  if (fgcolor == c) return;
  fgcolor = c;
  invalidate();
}

void PlStatus::setAntialias(int a) {
  if (antialias == a) return;
  antialias = a;
  invalidate();
}

void PlStatus::setFontsize(int fs) {
  if (fontsize == fs) return;
  fontsize = fs;
  invalidate();
}

void PlStatus::setBold(int b) {
  if (bold == b) return;
  bold = b;
  invalidate();
}

void PlStatus::setFont(const char *f) {
  if (STRCASEEQL(f, font)) return;
  font = f;
  invalidate();
}

void PlStatus::setOffsetX(int x) {
  if (offsetx == x) return;
  offsetx = x;
  invalidate();
}

void PlStatus::setOffsetY(int y) {
  if (offsety == y) return;
  offsety = y;
  invalidate();
}

int PlStatus::onPaint(Canvas *c) {

  RECT r;
  getClientRect(&r);

  COLORREF lbgcolor = bgcolor_set ? bkcolor : skin_bkcolor;
  c->fillRect(&r, lbgcolor);

  c->pushTextAntialias(antialias);

  COLORREF lfgcolor = fgcolor_set ? fgcolor : skin_fgcolor;
  c->pushTextColor(lfgcolor);

  c->pushTextFont(font);
  c->pushTextOpaque(0);
  c->pushTextBold(bold);
  c->pushTextSize(fontsize);

  int lenlen = offsetx;
  if (!len.isempty()) lenlen += c->getTextWidth(len);

  if (r.right - r.left >= lenlen) {
    c->textOut(r.right - lenlen, r.top+offsety, len);
  }

  c->textOutEllipsed(r.left+offsetx, r.top+offsety, r.right-r.left-offsetx*2-lenlen, r.bottom-r.top-offsety*2, title);

  c->popTextSize();
  c->popTextBold();
  c->popTextOpaque();
  c->popTextFont();
  c->popTextColor();
  c->popTextAntialias();
  return 1;
}

#if 0//CUT

int PlStatus::viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param, void *ptr, int ptrlen) {
  Editor *editor = dynamic_guid_cast<Editor>(item, classguid);
  if (editor != NULL) {
    ASSERT(editor == le_editor);
    switch (event) {
      case Editor::Event_REPAINT:
        setText(NULL); // refresh title
      break;
      case Editor::Event_PLAYLISTUNLOADED:
        title = NOFILE; len = NULL;
        invalidate();
        viewer_delAllItems();
        viewer_addViewItem(editor);
      break;
      case Editor::Event_PLAYLISTLOADED:
        title = NOFILE; len = NULL;
        invalidate();
        viewer_delAllItems();
        viewer_addViewItem(editor);
        viewer_addViewItem(editor->getPlaylist()->getDependencyPtr());
      break;
      default:
      return 0;
    }
    return 1;
  }
  Playlist *pl = dynamic_guid_cast<Playlist>(item, classguid);
  if (pl != NULL) {
    ASSERT(pl == le_editor->getPlaylist());
    switch (event) {
      case Playlist::Event_SETCURRENT:
        setText(pl);
      break;
      default:
        return 0;
    }
    return 1;
  }
  return 0;
}

int PlStatus::viewer_onItemDeleted(Dependent *item) { 
  prevpl=NULL;
  return 1; 
}

#endif

void PlStatus::setText(Playlist *pl) {
  if(pl==NULL) pl=prevpl;
  if(pl==NULL) return;
  prevpl=pl;
  int cur = pl->getCurrent();
  if (cur < 0) {
    title = NOFILE; len = NULL;
  } else {
    PlaylistEntry *entry = pl->enumEntry(cur);
    ASSERT(entry != NULL);
    title = StringPrintf("%d. %s", cur+1, entry->getCachedName());
    int lenlen = entry->getCachedLength();
    if (lenlen > 0) {
      char buf[900]="";
      TimeFmt::printMinSec(entry->getCachedLength()/1000, buf, 899);
      len = buf;
    } else {
      len = NULL;
    }
  }
  invalidate();
}

int PlStatus::corecb_onStatusMsg(const char *text) {
  title=text;
  len=NULL;
  invalidate();
  killTimer(100);
  setTimer(100,1000);
  return 0;
}

void PlStatus::timerCallback(int id) {
  if(id==100) {
    setText(NULL); // refresh playlist text
    killTimer(100);
  }
}

int PlStatus::corecb_onTitleChange(const char *title) {
  setText(NULL); // refresh playlist text
  return 0;
}

