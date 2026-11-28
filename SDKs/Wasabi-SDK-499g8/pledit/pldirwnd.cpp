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

#include "pldirwnd.h"

#include "svc_pldir.h"
#include "playlist.h"
#include "plitem2.h"
#include "editor.h"

#include "../studio/wac.h"

#include "../bfc/svc_enum.h"
#include "../common/dragitemi.h"
#include "../common/contextmenu.h"
#include "../common/contextcmd.h"
#include <bfc/attribs/attribs.h>
#include "../common/selectfile.h"
#include "../common/plstring.h"

PlDirWnd::PlDirWnd() : selected_playlist(INVALID_PLAYLIST_HANDLE), pldir(NULL),
  editor(NULL)
{
  setName(_("Loaded Playlists"));
  select_first = 0;
}

PlDirWnd::~PlDirWnd() {
  api->service_release(pldir);
  api->syscb_deregisterCallback(static_cast<SysCallbackI*>(static_cast<SkinCallbackI*>(this)));	// SkinCallback
}

int PlDirWnd::onInit() {
  PLDIRWND_PARENT::onInit();
  setHilitedColor("pledit.currentOutline");

  pldir = SvcEnumByGuid<svc_plDir>();

  if (pldir != NULL) {
    viewer_addViewItem(pldir->getDependencyPtr());

    int npl = pldir->getNumPlaylists();
    for (int i = 0; i < npl; i++) {
      PlaylistHandle hand = pldir->enumPlaylist(i);
      addTreeItem(new PlItem2(hand), NULL);
    }
  }

  api->syscb_registerCallback(static_cast<SysCallbackI*>(static_cast<SkinCallbackI*>(this)));	// SkinCallback

  // find editor
  GuiObject *ed = getGuiObject()->guiobject_findObjectByInterface(Editor::getInterfaceGuid());
  if (ed != NULL) {
    editor = static_cast<Editor*>(ed->guiobject_getRootWnd()->getInterface(Editor::getInterfaceGuid()));
    viewer_addViewItem(editor->getDependencyPtr());
  }

  return 1;
}

int PlDirWnd::onContextMenu(int x, int y) {
  if (pldir != NULL) {
    select_first = 1;	// force selection of first one entered
    ContextMenu(this, &DragItemT<svc_plDir>(pldir));
    select_first = 0;
  }
  return 1;
}

int PlDirWnd::viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param2, void *ptr, int ptrlen) {
  if (pldir != NULL && item == pldir->getDependencyPtr()) {
    switch (event) {
      case svc_plDir::Event_PLAYLIST_ADDED: {
        if (!pldir->getHidden(param2)) {
//          int sel_after = (enumAllItems(0) == NULL);
          TreeItem *ti = addTreeItem(new PlItem2(param2), NULL);
          if (/*sel_after ||*/ select_first) selectItemDeferred(ti);
          select_first = 0;
        }
      }
      break;
      case svc_plDir::Event_PLAYLIST_REMOVED: {
        PlItem2 *pli = getPlItemForPlaylist(param2);
        delItemDeferred(pli);
//CUT        PlItem2 *next = static_cast<PlItem2*>(pli->getSibling());
//CUT        if (next) selectItem(next);
      }
      break;
      case svc_plDir::Event_PLAYLIST_LABELCHANGE: {
        PlItem2 *pli = getPlItemForPlaylist(param2);
        if (pli != NULL) pli->resetLabel();
      }
      break;
      case svc_plDir::Event_PLAYLIST_VISIBILITY: {
        if (pldir->getHidden(param2)) {
          PlItem2 *pli = getPlItemForPlaylist(param2);
          if (pli != NULL) {
            delItemDeferred(pli);
          }
        } else {
          PlItem2 *pli = getPlItemForPlaylist(param2);
          if (pli == NULL) {
            int sel_after = (enumAllItems(0) == NULL);
            TreeItem *ti = addTreeItem(new PlItem2(param2), NULL);
            if (sel_after) selectItemDeferred(ti);
          }
        }
      }
      break;
    }
    return 1;
  } else if (editor != NULL && item == editor->getDependencyPtr()) {
    switch (event) {
      case Editor::Event_PLAYLISTLOADED: {
        PlaylistHandle hand = editor->getPlaylist()->getHandle();
        PlItem2 *plit = getPlItemForPlaylist(hand);
        selectItem(plit);
      }
      break;
    }
    return 1;
  }
  return 0;
}

int PlDirWnd::viewer_onItemDeleted(Dependent *item) {
  if (editor != NULL && item == editor->getDependencyPtr()) {
    editor = NULL;// bye bye
  }
  return 1;
}

PlItem2 *PlDirWnd::getPlItemForPlaylist(PlaylistHandle handle) {
  TreeItem *it;
  for (int i = 0; ; i++) {
    it = enumAllItems(i);
    if (it == NULL) break;
    PlItem2 *plit = static_cast<PlItem2 *>(it);
    if (plit->getPlaylistHandle() == handle) return plit;
  }
  return NULL;
}

int PlDirWnd::skincb_onLoaded() {
  PlaylistHandle l = pldir->enumPlaylist(0);
  PlItem2 *plit = getPlItemForPlaylist(l);
  selectItem(plit);
  return 1;
}

class PlDirSvcCmds : public ContextCmdT<svc_plDir> {
public:
  enum { NEWPL, LOADPL, CLEARALL };
  PlDirSvcCmds() {
    addCommandById("Create new playlist", NEWPL);
    addCommandById("Load playlist(s)...", LOADPL);
    addSeparator();
    addCommandById("Clear all loaded playlists", CLEARALL);
  }

  virtual void onCommand(svc_plDir* svc, int n, int cmdid) {
    switch (cmdid) {
      case NEWPL:
        svc->insertPlaylist(NULL);
      break;
      case LOADPL: {
        SelectFile sf(api->main_getRootWnd());
        sf.setIdent("playlist");
        sf.runSelector("files", TRUE, PlaylistExtensionStringR());
        if (sf.getNumFiles() > 0) {
          for (int i = 0; i < sf.getNumFiles(); i++) {
            PlaylistHandle hand = svc->insertPlaylist(sf.enumFilename(i));
          }
        }
      }
      break;
      case CLEARALL:
        svc->clearAll();
      break;
    }
  }
};

void PlDirWnd::registerServices(WAComponentClient *wac) {
  wac->registerService(new ContextCmdCreator<PlDirSvcCmds>);
}
