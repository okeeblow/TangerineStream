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
#include "plcmd.h"
#include "pldir.h"
#include "editor.h"
#include "editori.h"	//createNewEditor
#include "plitem.h"
#include "svc_pldir.h"
#include "../common/savefile.h"
#include "../common/plstring.h"
#include "../common/filename.h"
#include "../studio/api.h"
#include "../common/selectfile.h"
#include "../studio/services/svc_filesel.h"

int PlaylistCmd::getEnabled(DragItem *item, int n) {
  if (item->getNumData() == 0 || item->getDatum(0) == NULL) return 0;
  return 1;
}

PlCmdSort::PlCmdSort(const char *field, const char *name) :
  PlaylistCmd(StringPrintf("Sort by %s", name?name:StringToLower(field).getValue()), "Sort"), fieldname(field)
{ }

void PlCmdSort::onCommand(Playlist *item, int n, int cmdid) {
  item->sortBy(fieldname);
}

void PlaylistLoad::onCommand(Playlist *playlist, int n, int cmdid) {
  SelectFile sf(api->main_getRootWnd());
  sf.setIdent("playlist");
  sf.runSelector("files", FALSE, PlaylistExtensionStringR());
  if (sf.getNumFiles() > 0) {
    playlist->readFromFile(sf.enumFilename(0));
  }
}

void PlaylistSaveAs::onCommand(Playlist *item, int n, int cmdid) {
  if (item == NULL) return;
  SaveFileWnd save("playlist");
  if (save.getSaveFile(api->main_getRootWnd(), PlaylistExtensionStringW(), "m3u")) {
    item->writeToFile(save.getFilename(), TRUE);
  }
}

void PlaylistRemoveFromDir::onCommand(Playlist *item, int n, int cmdid) {
  if (item == NULL) return;
  svc_plDir *pldir = SvcEnumByGuid<svc_plDir>();
  if (pldir == NULL) return;
  pldir->removePlaylist(item->getHandle());
}

int PlaylistLock::getEnabled(DragItem *item, int n) {
  Playlist *pl = static_cast<Playlist*>(item->getDatum(n));
  if (pl == NULL) return 0;
  CfgItem *ci = pl->getCfgItem();
  if (ci == NULL) return 0;
  if (ci->getDataAsInt("Locked")) return 0;	// can't fux0r if system locked
  return 1;
}

int PlaylistLock::getChecked(DragItem *item, int n) {
  Playlist *pl = static_cast<Playlist*>(item->getDatum(n));
  if (pl == NULL) return 0;
  CfgItem *ci = pl->getCfgItem();
  if (ci == NULL) return 0;
  return ci->getDataAsInt("UserLocked");
}

void PlaylistLock::onCommand(Playlist *item, int n, int cmdid) {
  CfgItem *ci = item->getCfgItem();
  if (ci == NULL) return;
  ci->setDataAsInt("UserLocked", !ci->getDataAsInt("UserLocked"));
}

PlaylistSendTo::PlaylistSendTo() : ContextCmdT<Filename>("Send to playlist") {
  addpl = NULL;
}

void PlaylistSendTo::populateCommands() {
  addCommandById("Create new playlist", -10);
  svc_plDir *pldir = SvcEnumByGuid<svc_plDir>();
  if (pldir == NULL) return;
  int n = pldir->getNumPlaylists();
  if (n > 0) addSeparator();
  for (int i = 0; i < n; i++) {
    PlaylistHandle hand = pldir->enumPlaylist(i);
    const char *label = pldir->getPlaylistLabel(hand);
    if (label == NULL) label = "NULLLLLLLLL";
    addCommandById(label, hand);
  }
  SvcEnum::release(pldir);
}

void PlaylistSendTo::onCommand(Filename *item, int n, int cmdid) {
  svc_plDir *pldir = SvcEnumByGuid<svc_plDir>();
  ASSERT(pldir != NULL);
  if (pldir == NULL) return;
  Playlist *pl = NULL;
  if (cmdid == -10) {	// create
    if (n == 0) {
      PlaylistHandle hand = pldir->insertPlaylist();
      EditorI::createNewEditor(hand);
      addpl = pldir->getPlaylist(hand);
    }
    pl = addpl;
  } else {
    pl = pldir->getPlaylist(cmdid);
  }
  if (pl != NULL) {
    int npl = pl->getNumEntries();
    pl->addPlaylistItem(item->getFilename());
    if (n == 0) pl->ensureVisible(npl);
  }
  SvcEnum::release(pldir);
}

PlaylistAdd::PlaylistAdd() : ContextCmdT<Playlist>("Add") {
}

PlaylistAdd::~PlaylistAdd() {
  foreach(fslist)
    SvcEnum::release(fslist.getfor());
  endfor
}

void PlaylistAdd::populateCommands() {
  FileSelectorEnum fse(NULL);
  svc_fileSelector *svc;
  for (svc = fse.getFirst(); svc != NULL; svc = fse.getNext()) {
    fslist.addItem(svc);
    addCommandById(StringPrintf("Add %s...", svc->getPrefix()), fslist.getNumItems()-1, NULL, TRUE, FALSE, ContextCmdSortVal::MIDDLE-1);
  }
}

void PlaylistAdd::onCommand(Playlist *item, int n, int cmdid) {
  svc_fileSelector *svc = fslist[cmdid];
  ASSERT(svc != NULL);
  svc->setExtList(api->core_getExtSupportedExtensions());
  api->pushModalWnd();
  svc->runSelector(api->main_getRootWnd(), FileSel::OPEN, TRUE, svc->getPrefix());
  api->popModalWnd();
  int nf = svc->getNumFilesSelected();
  for (int i = 0; i < nf; i++) {
    const char *ps = svc->enumFilename(i);
    if (api->metadb_addNewItem(ps)) {
      item->addPlaylistItem(ps);
    }
    if (i == 0) item->ensureVisible(nf);
  }
}
