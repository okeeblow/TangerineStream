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
#include "pldirsvc.h"
#include "pledit.h"
#include "playlisti.h"
#include "plslot.h"
#include "editor.h"

#include <common/contextcmd.h>
#include <studio/wac.h>
#include <studio/services/creators.h>
#include <common/filename.h>

#include <common/selectfile.h>
#include <common/plstring.h>
#include <bfc/parse/cmdline.h>

#include <bfc/attribs/attribs.h>

class DataDir : public String {
public:
  DataDir() : String(api->path_getComponentDataPath()) {
    cat(DIRCHARSTR);
    cat("pledit");
  }
};
#define DATADIR (DataDir())

enum { T_REMOVEPLAYLIST=10, T_PROCESSCMDLINE=20, T_NOMOFILES=30 };

PlDirSvc *thepldirsvc=NULL;

PlDirSvc::PlDirSvc() {
  extern WACpledit wacpledit;
  wacpledit.registerResource(this);
  ASSERT(thepldirsvc == NULL);
  thepldirsvc = this;
  currentlyopen = INVALID_PLAYLIST_HANDLE;
  force_enqueue = 0;
  auto_start = 0;
}

PlDirSvc::~PlDirSvc() {
  ASSERT(thepldirsvc == this);
  thepldirsvc = NULL;
  files.deleteAll();
}

void PlDirSvc::onRegisterServices() {
  api->syscb_registerCallback(this);
}

void PlDirSvc::onDeregisterServices() {
  api->syscb_deregisterCallback(this);
}

int PlDirSvc::getNumPlaylists(int skip_hidden) {
  if (!skip_hidden) return directory.getNumItems();
  int count = 0;
  foreach(directory)
    if (!directory.getfor()->getHidden()) count++;
  endfor
  return count;
}

PlaylistHandle PlDirSvc::enumPlaylist(int n, int skip_hidden) {
  if (!skip_hidden) {
    PlaylistSlot *pls = directory[n];
    if (pls == NULL) return INVALID_PLAYLIST_HANDLE;
    return pls->getHandle();
  }
  PlaylistSlot *pls = NULL;
  int count=0;
  foreach(directory)
    if (directory.getfor()->getHidden()) continue;
    if (count == n) {
      pls = directory.getfor();
      break;
    }
    count++;
  endfor
  return (pls == NULL) ? INVALID_PLAYLIST_HANDLE : pls->getHandle();
}

Playlist *PlDirSvc::getPlaylist(PlaylistHandle handle) {
  PlaylistSlot *slot = slotFromHandle(handle);
  return (slot != NULL) ? slot->getPlaylist() : NULL;
}

void PlDirSvc::ungetPlaylist(Playlist *pl) {
  // decrease ref count
}

const char *PlDirSvc::getPlaylistLabel(PlaylistHandle handle) {
  PlaylistSlot *slot = slotFromHandle(handle);
  if (slot == NULL) return NULL;
  return slot->getLabel();
}

int PlDirSvc::setPlaylistLabel(PlaylistHandle handle, const char *newlabel) {
  PlaylistSlot *slot = slotFromHandle(handle);
  if (slot == NULL) return 0;
  slot->setLabel(newlabel);
  dependent_sendEvent(svc_plDir::depend_getClassGuid(), Event_PLAYLIST_LABELCHANGE, handle);
  return 1;
}

PlaylistHandle PlDirSvc::insertPlaylist(const char *filename, const char *label, const char *path, int hidden){
  String tl;
  if (label == NULL) {
    tl = makeNewPlName();
    label = tl;
  }
  return addPlaylist(new PlaylistSlot(filename, inventFileName(), label, hidden));
}

int PlDirSvc::removePlaylist(PlaylistHandle handle) {
  api->pushModalWnd();
  timerclient_postDeferredCallback(T_REMOVEPLAYLIST, handle, 0);
  return 1;
}

int PlDirSvc::setAutoSave(PlaylistHandle handle, int autosave) {
  PlaylistSlot *slot = slotFromHandle(handle);
  if (slot == NULL) return 0;
  slot->setAutoSave(autosave);
  return 1;
}

int PlDirSvc::timerclient_onDeferredCallback(int param1, int param2) {
  switch (param1) {
    case T_REMOVEPLAYLIST: {
      api->popModalWnd();
      PlaylistHandle handle = param2;
      PlaylistSlot *slot = slotFromHandle(handle);
      ASSERT(slot != NULL);
      if (slot == NULL) return 0;
      delPlaylist(slot);
    }
    break;
    case T_PROCESSCMDLINE: {
      processCmdLine(WASABI_API_APP->main_getCommandLine(), 0);

      const GUID options_guid = 
      { 0x280876cf, 0x48c0, 0x40bc, { 0x8e, 0x86, 0x73, 0xce, 0x6b, 0xb4, 0x62, 0xe5 } };
      CfgItem *options=WASABI_API_CONFIG->config_getCfgItemByGuid(options_guid);
      if (auto_start || options && _intVal(options, "Auto-play at startup")) {
        PlaylistHandle handle = getCurrentlyOpen();
        if(handle == INVALID_PLAYLIST_HANDLE) handle = enumPlaylist(0);
        if (handle != INVALID_PLAYLIST_HANDLE) {
          Playlist *pl = getPlaylist(handle);
          if(pl) {
            // check if core is not already playing
            if(api->core_getStatus(0)==0) pl->startPlayback();
          }
        }
      }
    }
    break;
  }
  return 1;
}

void PlDirSvc::timerclient_timerCallback(int id) {
  switch (id) {
    case T_NOMOFILES: {
      api->popModalWnd();
      PlaylistHandle handle = getCurrentlyOpen();
      if (handle == INVALID_PLAYLIST_HANDLE) handle = insertPlaylist();
      Playlist *pl = getPlaylist(handle);
      int nf = pl->getNumEntries();

      extern _bool enqueue;
      if (!enqueue && !force_enqueue)
        pl->deleteAll();

      foreach(files)
        api->metadb_addNewItem(*files.getfor());
        pl->addPlaylistItem(*files.getfor());
      endfor
      files.deleteAll();
      if (!enqueue && !force_enqueue) {
        pl->startPlayback();
      }
      timerclient_killTimer(T_NOMOFILES);
      force_enqueue = 0;
    }
    break;
    default:
      TimerClientI::timerclient_timerCallback(id);
  }
}

void PlDirSvc::processCmdLine(const char *cmdline, int seqno) {
//  OutputDebugString(cmdline);
  CmdLine c(cmdline);

  auto_start = c.hasSwitch("autostart");
  if (c.hasSwitch("add")) force_enqueue = 1;

  int nfound = 0;

  // add all naked entries as files
  for (int i = 0; i < c.getNumStrings(); i++) {
    if (!c.isSwitch(i)) {
      String param = c.enumString(i);
      String *file = new String;
      if(!STRSTR(param,"://")) file->printf("file:%s", param.getValue());
      else file->setValue(param);
      files.addItem(file);
      nfound ++;
    }
  }

  if (nfound > 0) {
    if (!api->getModalWnd()) api->pushModalWnd();
    timerclient_killTimer(T_NOMOFILES);
    timerclient_setTimer(T_NOMOFILES, 400);
  }
}

void PlDirSvc::clearAll() {
  foreach(directory)
    if (!directory.getfor()->getHidden()) delPlaylist(directory.getfor());
  endfor
}

int PlDirSvc::setHidden(PlaylistHandle handle, int hidden) {
  PlaylistSlot *slot = slotFromHandle(handle);
  if (slot == NULL) return 0;
  if (hidden != slot->getHidden()) {
    slot->setHidden(hidden);
    dependent_sendEvent(svc_plDir::depend_getClassGuid(), svc_plDir::Event_PLAYLIST_VISIBILITY, handle);
  }
  return 1;
}

int PlDirSvc::getHidden(PlaylistHandle handle) {
  PlaylistSlot *slot = slotFromHandle(handle);
  if (slot == NULL) return -1;
  return slot->getHidden();
}

void PlDirSvc::runlevelcb_onAppRunning() {
  // load up playlists
  int npl = WASABI_API_CONFIG->getIntPrivate("pldir/nplaylists", 0);
  for (int i = 0; i < npl; i++) {
    char filename[WA_MAX_PATH]="";
    StringPrintf tag("pldir/loaded playlists/%d/filename", i);
    WASABI_API_CONFIG->getStringPrivate(tag, filename, sizeof filename, "");
    char label[WA_MAX_PATH]="";
    tag.printf("pldir/loaded playlists/%d/label", i);
    WASABI_API_CONFIG->getStringPrivate(tag, label, sizeof label, "");
    if (STRLEN(filename) >= 1 && FEXISTS(filename)) {
      addPlaylist(new PlaylistSlot(filename, filename, label));
    }
  }

  // callback to check command line & auto-start
  timerclient_postDeferredCallback(T_PROCESSCMDLINE, 0, 10);
}

void PlDirSvc::runlevelcb_onShutdown() {
  // save out playlists
  int prev_npl = WASABI_API_CONFIG->getIntPrivate("pldir/nplaylists", 0);

  Std::createDirectory(DATADIR);
 
  int count = 0;

  // keep track of their saved out filenames
  foreach(directory)
    PlaylistSlot *slot = directory.getfor();
    if (!slot->getAutoSave()) continue;
    StringPrintf tag("pldir/loaded playlists/%d/filename", count);
    WASABI_API_CONFIG->setStringPrivate(tag, slot->getSaveFileName());
    tag.printf("pldir/loaded playlists/%d/label", count);
//    ASSERT(slot->getLabel() != NULL);
    WASABI_API_CONFIG->setStringPrivate(tag, slot->getLabel());
    slot->save();//write the backup file (with no metadata)
    count ++;
  endfor
  WASABI_API_CONFIG->setIntPrivate("pldir/nplaylists", count);

  // delete slots
  directory.deleteAll();
}

PlaylistSlot *PlDirSvc::slotFromHandle(PlaylistHandle handle) {
  foreach(directory)
    if (directory.getfor()->getHandle() == handle) return directory.getfor();
  endfor
  return NULL;
}

PlaylistHandle PlDirSvc::addPlaylist(PlaylistSlot *pls) {
  ASSERT(directory.haveItem(pls) == 0);
  directory.addItem(pls);
  // do notiy here
  dependent_sendEvent(svc_plDir::depend_getClassGuid(), svc_plDir::Event_PLAYLIST_ADDED, pls->getHandle());
  return pls->getHandle();
}

void PlDirSvc::delPlaylist(PlaylistSlot *pls) {
  ASSERT(pls != NULL);
  ASSERT(directory.haveItem(pls));
  if (currentlyopen == pls->getHandle()) {
    currentlyopen = INVALID_PLAYLIST_HANDLE;
  }
  directory.removeItem(pls);
  // do notify here
  dependent_sendEvent(svc_plDir::depend_getClassGuid(), svc_plDir::Event_PLAYLIST_REMOVED, pls->getHandle());
  // and delete it here
  FDELETE(pls->getSaveFileName());
  delete pls;

  // always make sure there's at least one nonhidden playlist
  if (enumPlaylist(0) == INVALID_PLAYLIST_HANDLE) {
    insertPlaylist();
  }
}

String PlDirSvc::inventFileName() {
  String ret;
  DataDir dd;
  Std::createDirectory(dd);
  for (int n = 0; n < 20000; n++) {
    ret.printf("%s%s%08d.m3u", dd.getValue(), DIRCHARSTR, Std::random());
    if (FEXISTS(ret)) continue;
    break;
  }
  if (n == 20000) OutputDebugString("could not make up a filename after trying 20k different ones :(\n");
  return ret;
}

int PlDirSvc::dragDrop(RootWnd *sourceWnd, int x, int y) {
  int nitems, pos;

  int num_added = 0;

  if ((pos = DI(sourceWnd).dragCheckData(DD_FILENAME, &nitems)) >= 0) {

    PlaylistHandle hand = getCurrentlyOpen();
    if (hand == INVALID_PLAYLIST_HANDLE)
      hand = insertPlaylist(NULL, makeNewPlName());
    Playlist *pl = getPlaylist(hand);

    int prev_nitems = pl->getNumEntries();

    extern _bool enqueue;
    if (!enqueue) {
    // clear out old playlist
      pl->deleteAll();
    }

    int offset = pl->getNumEntries();

    for (int i = 0; i < nitems; i++) {
      Filename *fn = (Filename *)DI(sourceWnd).dragGetData(pos, i);
      const char *playstring = fn->getFilename();
      if (api->metadb_addNewItem(playstring)) {
        pl->addPlaylistItem(playstring);
        num_added++;
      }
    }

    if (num_added && !enqueue) {
      pl->setCurrent(offset);
      pl->startPlayback();
    }
//CUT    extern GUID component_guid;
//CUT    if (!api->skinwnd_enumByGuid(component_guid, 0)) {
//CUT      api->skinwnd_createByGuid(component_guid);
//CUT    }
  }
  return 0;
}

void PlDirSvc::setCurrentlyOpen(PlaylistHandle handle) {
  currentlyopen = handle;
}

PlaylistHandle PlDirSvc::getCurrentlyOpen() {
  return currentlyopen;
}

PlaylistHandle PlDirSvc::getCurrentlyPlaying() {
return INVALID_PLAYLIST_HANDLE;
#if 0
  PlaylistSequencer *seq = WACpledit::getPlaylistSequencer();
  if (seq == NULL) return INVALID_PLAYLIST_HANDLE;
  Playlist *pl = seq->getPlaylist();
  if (pl == NULL) return INVALID_PLAYLIST_HANDLE;
  return pl->getHandle();
#endif
}

const char *PlDirSvc::getPlaystringFromEntry(PlaylistEntry *entry) {
  return entry->getPlaystring();
}

String PlDirSvc::makeNewPlName() {
  extern _string newplname;
  String ret;
  for (int i = 1; ; i++) {
    int again = 0;
    ret.printf(newplname, i);
    foreach(directory)
      if (STRCASEEQL(ret, directory.getfor()->getLabel())) {
        again = 1;
        break;
      }
    endfor
    if (!again) break;
  }
  return ret;
}
