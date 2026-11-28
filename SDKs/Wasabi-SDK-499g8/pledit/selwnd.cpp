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

#include "selwnd.h"

#include "../common/playstring.h"
#include "../common/metatags.h"
#include "playlist.h"
#include "svc_pldir.h"
#include "editor.h"

SelWnd::SelWnd() :
  need_rescan(0), pldir(NULL)
{ }

SelWnd::~SelWnd() {
  SvcEnum::release(pldir);
}

int SelWnd::onInit() {
  SELWND_PARENT::onInit();

  getGuiObject()->guiobject_setId("pledit.selection.window");

  pldir = SvcEnumByGuid<svc_plDir>();

  ItemListColumn *ilc = new ItemListColumn_MetaTag(MT_NAME); 
  ilc->setDynamic(TRUE);
  insertColumn(ilc, 30);

  showColumnsHeaders = FALSE;

  setName(getWindowTypeName());
  GuiObject *ed = getGuiObject()->guiobject_findObjectByInterface(Editor::getInterfaceGuid());
  if (ed != NULL) {
    Editor *e = static_cast<Editor*>(ed->guiobject_getRootWnd()->getInterface(Editor::getInterfaceGuid()));
    ASSERTPR(e != NULL, "something labeled playlist editor didn't implement the interface");
    if (e != NULL) {
      viewer_addViewItem(e);
    }
  }
  return 1;
}

void SelWnd::onDoubleClick(int itemnum) {
  Editor *ed = viewer_enumViewItem(0);
  ASSERT(ed != NULL);
  Playlist *pl = ed->getPlaylist();
  ASSERT(pl != NULL);
  PlaylistEntry *entry = reinterpret_cast<PlaylistEntry *>(getItemData(itemnum));
  ASSERT(entry != NULL);
  pl->startPlayback(pl->getPositionForEntry(entry));
}

void SelWnd::onLeftClick(int itemnum) {
  Editor *ed = viewer_enumViewItem(0);
  ASSERT(ed != NULL);
  PlaylistEntry *entry = reinterpret_cast<PlaylistEntry *>(getItemData(itemnum));
  Playlist *pl = ed->getPlaylist();
  ASSERT(pl != NULL);
  int pos = pl->getPositionForEntry(entry);
  ed->ensureVisible(pos);
  ed->enqueueItem(pos, FALSE);
}

int SelWnd::viewer_onEvent(Editor *item, int event, int param2, void *ptr, int ptrlen) {
  Editor *ed = item;
  switch (event) {
    case Editor::Event_ENTRYSELECTED:
    case Editor::Event_ENTRYDESELECTED:
    case Editor::Event_PLAYLISTUNLOADED:
      if (!need_rescan) {
        need_rescan = 1;
        postDeferredCallback(10, 0); 
      }
    break;
  }
  return 1;
}

int SelWnd::onDeferredCallback(int p1, int p2) {
  switch (p1) {
    case 10: {
      ASSERT(need_rescan);
      need_rescan = 0;

      deleteAllItems();
// don't allow repaint
//      EntryList list;
      Editor *ed = viewer_enumViewItem(0);
      ASSERT(ed != NULL);
      Playlist *pl = ed->getPlaylist();
      if (pl != NULL) {
        int n = pl->getNumEntries();
        for (int i = 0; i < n; i++) {
          if (ed->itemSelected(i)) {
            addItem(NULL, reinterpret_cast<LPARAM>(pl->enumEntry(i)));
          }
        }
      }
// allow repaint

    }
    return 1;
  }
  return 0;
}

const char *SelWnd::convertlParam(LPARAM lParam) {
  ASSERT(lParam != 0);
  PlaylistEntry *entry = reinterpret_cast<PlaylistEntry*>(lParam);
  return pldir->getPlaystringFromEntry(entry);
}
