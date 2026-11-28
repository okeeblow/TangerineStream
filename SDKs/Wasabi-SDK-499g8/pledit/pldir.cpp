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

#include "pldir.h"

#include "../bfc/rootwnd.h"
#include "editori.h"
#include "../studio/wac.h"
#include "../common/selectfile.h"
#include "../common/plstring.h"
#include "../common/contextmenu.h"

class PlDirCmds : public ContextCmdObjDir {
public:
  enum { NEWPL, LOADPL, CLEARALL };
  PlDirCmds() : ContextCmdObjDir("playlist") {
    addCommandById("Create new playlist", NEWPL);
    addCommandById("Load playlist(s)...", LOADPL);
    addSeparator();
    addCommandById("Clear all loaded playlists", CLEARALL);
  }
  virtual void onCommand(svc_objectDir* svc, int n, int cmdid) {
    switch (cmdid) {
      case NEWPL: {
        PlaylistDir *d = static_cast<PlaylistDir*>(svc);
        ObjectHandle h = d->insertObject(NULL, "1.8.7");
        PlaylistI *pl = d->getTypedObject(h);
        d->setObjectPath(h, "All Tracks/Artist");
        pl->addPlaylistItem("query://Artist == 1.8.7");
      }
      break;
      case LOADPL: {
        SelectFile sf(api->main_getRootWnd());
        sf.setIdent("playlist");
        sf.runSelector("files", TRUE, PlaylistExtensionStringR());
        if (sf.getNumFiles() > 0) {
          for (int i = 0; i < sf.getNumFiles(); i++) {
            svc->insertObject(sf.enumFilename(i), "blap");
          }
        }
      }
      break;
      case CLEARALL:
//FUCKO: are you SUUUUUUURE
        svc->clearAll();
      break;
    }
  }
};

PlaylistDir::PlaylistDir() : PLAYLISTDIR_PARENT("playlist", "Playlist Directory") {
  ObjectHandle h = insertObject(NULL, "");
  setObjectIcon(h, "pledit.line");
  setObjectSelectable(h, FALSE);
  setObjectSortOrder(h, 16336);
}

void PlaylistDir::objdir_onObjectCreation(PlaylistI *obj, ObjectHandle handle) {
  obj->setPlaylistHandle(handle);
  if (obj->getPlaylistName() == NULL)	// set playlist name to label IF NULL
    obj->setPlaylistName(getObjectLabel(handle));
}

int PlaylistDir::onAction(int action, RootWnd *from, const char *target, ObjectHandle handle) {
  switch (action) {
    case ODACTION_SELECTED: {
//CUT      if (STREQL(getObjectLabel(), "")) return 0;
      RootWnd *rw = NULL;
      if (!String(target).isempty()) {
        rw = from->findWindow(target);
      } else {
        rw = from->findWindowByInterface(Editor::getInterfaceGuid());
      }
      if (rw == NULL) return 0;
      Editor *ed = static_cast<Editor *>(rw->getInterface(Editor::getInterfaceGuid()));
      if (ed == NULL) return 0;
      RECT wr;
      from->getWindowRect(&wr);
      ed->setPlaylistByHandle(handle, &wr);
    }
    return 1;
  }
  return 0;
}

int PlaylistDir::contextMenu(RootWnd *from, int x, int y, ObjectHandle handle) {
  Playlist *pl = getTypedObject(handle);
  DragItemT<Playlist> di(pl);
  from->clientToScreen(&x, &y);
  ContextMenu cm(from, x, y, &di, false);
  cm.addSeparator();
  cm.addCommand("Rename playlist", -100);
  cm.addCommand("Open in new window", -200);
  int r = cm.popAtMouse();
  switch (r) {
    case -100:
      return -1;

    case -200:
      EditorI::createNewEditor(handle);
    break;
  }
  return 1;
}

void pldir_registerServices(WAComponentClient *wac) {
  // the dir itself
  wac->registerService(new ObjectDirCreator<PlaylistDir>);
  // context cmds for it
  wac->registerService(new ContextCmdCreator<PlDirCmds>);
}
