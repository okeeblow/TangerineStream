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

#include "playlisti.h"

#include "pledit.h"
#include "pledit.h"
#include "plcmd.h"
#include "plload.h"
#include "queryload.h"
#include "dirload.h"

#include "../studio/assert.h"
#include "../studio/api.h"
#include "../common/metatags.h"
#include "../common/stringtable.h"
#include <core/corehandle.h>

#include <bfc/attribs/attrint.h>

#include "../studio/services/creators.h"

#define PLAYLIST_VER	"1"

const int PTRLIST_INC = 4090;

static Freelist<PlaylistEntry> plentry_freelist;

// table of names
static StringTable name_cache;

PlaylistEntry::PlaylistEntry(const char *_playstring)
  : playstring(_playstring),
  current(0), wascurrent(0), enabled(0), user_enabled(1),
  cached_name(NULL), cached_length(-1),
  shuffleid(-Std::random32(9999999)), prev_shuffleid(-1), rendermode(0)
{
  ASSERT(_playstring != NULL);
}

PlaylistEntry::~PlaylistEntry() {
  delCachedInfo();
}


const char *PlaylistEntry::getCachedName(int autoread) {
  if (cached_name != NULL) return cached_name;
  if (!autoread) return NULL;
  // otherwise read it and cache it
  rendermode = 0;
  char buf[4096]="";
  int r = api->metadb_getMetaData(playstring, MT_NAME, buf, 4096, MDT_STRINGZ);
  if (r <= 0) {
    const char *f = playstring;
    if (!STRNINCMP(playstring, "file:")) {
      f = Std::filename(playstring);
      ASSERT(f != NULL);
    }
    STRNCPY(buf, f, sizeof(buf)-1);
    rendermode = 1;
  }
  buf[sizeof(buf)-1] = 0;
  cached_name = name_cache.allocStr(buf);

  // grab length too
  cached_length = -1;
  api->metadb_getMetaData(playstring, MT_LENGTH, (char *)&cached_length, 4, MDT_TIME);

  // and disabled status
  int tmp = 0;
  api->metadb_getMetaData(playstring, MT_DISABLED, (char*)&tmp, 4, MDT_BOOLEAN);
  enabled = !tmp;

  return cached_name;
}

int PlaylistEntry::getCachedLength(int autoread) {
  getCachedName(autoread);
  return cached_length;
}

int PlaylistEntry::getCachedEnabled() {
  getCachedName();
  return (enabled && user_enabled);
}

void PlaylistEntry::delCachedInfo() {
  name_cache.deallocStr(cached_name); cached_name = NULL;
}

PlaylistI::PlaylistI(const char *loadfrom, int full_metadata) :
last_pos(0), modified(0), handle(INVALID_PLAYLIST_HANDLE),
_enable_metadata(TRUE), _enable_database_add(TRUE), corename("main"),
plseq(new PlaylistSequencer)
{
  registerAttribute(new _string("Name", "blah"));
  registerAttribute(new _int("Locked", FALSE));
  registerAttribute(new _int("UserLocked", FALSE));
  registerAttribute(new _bool("Unloadable", TRUE));// by the user that is
  if (loadfrom != NULL) {
    readFromFile(loadfrom, full_metadata, full_metadata);
  }
}

PlaylistI::~PlaylistI() {
  deleteAll();
  if (plseq != NULL) CoreHandle(corename).deregisterSequencer(plseq);
  delete plseq;
}

void *PlaylistI::dependent_getInterface(const GUID *classguid) {
  HANDLEGETINTERFACE(Playlist);
  HANDLEGETINTERFACE(CfgItem);
  return NULL;
}

const char *PlaylistI::getPlaylistName() {
  return CfgItemI::cfgitem_getName();
}

void PlaylistI::setPlaylistName(const char *newname) {
  CfgItemI::setName(newname);
}

void PlaylistI::cfgitem_onSetName() {
  dependent_sendEvent(Playlist::depend_getClassGuid(), Playlist::Event_NAMECHANGE);
}

int PlaylistI::getNumEntries() {
  return entries.getNumItems();
}

int PlaylistI::getPositionForEntry(PlaylistEntry *entry) {
  return entries.searchItem(entry);
}

const char *PlaylistI::getCachedNameForEntry(PlaylistEntry *entry, int autoread) {
  if (entry == NULL) return NULL;
  return entry->getCachedName(autoread);
}

int PlaylistI::getCachedLengthForEntry(PlaylistEntry *entry, int autoread) {
  if (entry == NULL) return -1;
  return entry->getCachedLength(autoread);
}

PlaylistEntry *PlaylistI::enumEntry(int which) {
  return entries[which];
}

const char *PlaylistI::getPlayStringFromEntry(PlaylistEntry *entry) {
  if (entry == NULL) return NULL;
  return entry->getPlaystring();
}

void PlaylistI::entryEnable(PlaylistEntry *entry, int enabled) {
  if (entry == NULL) return;
  int pos = getPositionForEntry(entry);
  if (pos < 0) return;
  entry->user_enabled = !!enabled;
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_REPAINT, pos);
}

int PlaylistI::entryEnabled(PlaylistEntry *entry) {
  if (entry == NULL) return 0;
  return entry->user_enabled;
}

const char *PlaylistI::enumItem(int which) {
  PlaylistEntry *ent = entries[which];
  if (ent != NULL)
    return ent->getPlaystring();
  else
    return NULL;
}


int PlaylistI::addPlaylistItem2(const char *playstring, int after) {
  return addPlaylistItem(playstring, after, TRUE);
}

int PlaylistI::addPlaylistItem(const char *playstring, int after, int expand) {
  ASSERT(after >= APPEND);
  if (after == APPEND || after >= entries.getNumItems()) after = entries.getNumItems()-1;

  int afterc = after;
  return _addPlaylistItem(playstring, afterc, expand, 0);
}

int PlaylistI::_addPlaylistItem(const char *playstring, int &after, int expand, int recursionlevel) {
  if (playstring == NULL) return 0;
  ASSERT(after != APPEND);

  int expanded = 0;

  // try to expand it as a playlist
  if (expand) {
    PlaylistReaderEnum pre(playstring);
    svc_playlistReader *svc = pre.getNext(FALSE);
    if (svc != NULL && svc->readPlaylist(playstring) >= 0) {
//FUCKO   svc->enableMetadata(_enable_metadata);
      svc->enableDatabaseAdd(_enable_database_add);
      int n = svc->getNumEntries();
      for (int i = 0; i < n; i++) {
        const char *ps = svc->enumEntry(i);
        if (ps != NULL) {
          _addPlaylistItem(ps, after, expand, recursionlevel+1);
          after++;
        }
      }
      // set name to match first playlist encountered
      if (recursionlevel == 0 && getPlaylistName() == NULL) setName(svc->getLabel());
      expanded = 1;
    }
    if (svc) pre.getLastFactory()->releaseInterface(svc);
  }

  if (!expand || !expanded) {
  // do the actual add
    PlaylistEntry *entry = newPlaylistEntry(playstring);
    entries.addItem(entry, PTRLIST_POS_LAST, PTRLIST_INC);

    // move it into place
    if (entries.getNumItems() != 1 && after != APPEND) {
      entries.moveItem(entries.getNumItems()-1, after+1);
    }
    onItemAdd(entry);
  }

  return 1;
}

int PlaylistI::removeByEntry(PlaylistEntry *entry) {
  ASSERT(entry != NULL);

  int pos = entries.searchItem(entry);
  if (pos < 0) return 0;

  entries.removeItem(entry);
  delPlaylistEntry(entry);	// delete

  return 1;
}

int PlaylistI::removeByPos(int nentry) {

  if (nentry < 0 || nentry >= entries.getNumItems()) return 0;

  PlaylistEntry *entry = entries[nentry];

  entries.removeByPos(nentry);

  delPlaylistEntry(entry);	// delete

  return 1;
}

int PlaylistI::removeByPlaystring(const char *playstring) {
  int pos = -1;
  for (int i = 0; i < entries.getNumItems(); i++) {
    if (STREQL(entries[i]->getPlaystring(), playstring)) {
      pos = i;
      break;
    }
  }
  if (pos < 0) return 0;
  removeByPos(pos);
  return 0;
}

void PlaylistI::deleteAll() {
//CUT  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_BEGINDELETEALL);
  for (int i = 0; i < entries.getNumItems(); i++) {
    delPlaylistEntry(entries[i]);	// delete it
  }
  entries.removeAll();
//CUT  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_ENDDELETEALL);
}

void PlaylistI::ensureVisible(int pos) {
  if (pos < 0 || pos >= getNumEntries()) return;
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_ENSUREVISIBLE, pos);
}

int PlaylistI::cloneFrom(Playlist *pl) {
  deleteAll();
  int n = pl->getNumEntries();
  if (n <= 0) return 1;
  entries.setMinimumSize(n);
  for (int i = 0; i < n; i++) {
    addPlaylistItem(pl->enumItem(i));
  }
  return n;
}

int PlaylistI::setCurrent(int pos) {
  int i;
  // unset currentness
  for (i = 0; i < entries.getNumItems(); i++)
    entries[i]->setCurrent(0);

  if (pos < 0 || pos >= entries.getNumItems()) return 0;

  entries[pos]->setCurrent(1);

  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_SETCURRENT, pos);
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_REPAINT, -1);

  return 1;
}

int PlaylistI::getCurrent() {
  for (int i = 0; i < entries.getNumItems(); i++)
    if (entries[i]->getCurrent()) return i;
  return -1;
}

void PlaylistI::onNextFile() {
  for (int i = 0; i < entries.getNumItems(); i++)
    entries[i]->setWasCurrent(0);
  int cur = getCurrent();
  if (cur < 0) return;
  entries[cur]->setWasCurrent(1);
}

void PlaylistI::markNextFile(int pos, int insert_at_front) {
  if (pos < 0 || pos >= entries.getNumItems()) return;
  playback_queue.removeItem(entries[pos]);
  playback_queue.addItem(entries[pos]);	// make it appear on the end
  if (insert_at_front)	// or the front :)
    playback_queue.moveItem(playback_queue.getNumItems()-1, 0);
  // notify there was a queue change
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_QUEUE_ADDITION, pos);
  // invalidate
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_REPAINT, -1);
}

void PlaylistI::unmarkFile(int pos) {
  if (pos < 0 || pos >= entries.getNumItems()) return;
  if (playback_queue.haveItem(entries[pos]))
    dependent_sendEvent(Playlist::depend_getClassGuid(), Event_QUEUE_DELETION, pos);
  playback_queue.removeItem(entries[pos]);
  if (playback_queue.getNumItems() == 0)
    dependent_sendEvent(Playlist::depend_getClassGuid(), Event_QUEUE_EMPTY);
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_REPAINT, -1);
}

const char *PlaylistI::getNextQueuedFile(int *pos) {
  if (playback_queue.getNumItems() <= 0) {
    playback_queue.removeAll();
    return NULL;
  }
  PlaylistEntry *ret = playback_queue[0];
  playback_queue.removeByPos(0);

  if (pos != NULL) *pos = getPositionForEntry(ret);

  // notify if queue empties
  if (playback_queue.getNumItems() == 0)
    dependent_sendEvent(Playlist::depend_getClassGuid(), Event_QUEUE_EMPTY);

  // invalidate
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_REPAINT, -1);

  return ret->getPlaystring();
}

int PlaylistI::getQueuePos(int pos) {
  PlaylistEntry *entry = enumEntry(pos);
  return playback_queue.searchItem(entry);
}

int PlaylistI::getNumQueued() {
  return playback_queue.getNumItems();
}

int PlaylistI::replaceQueueEntry(PlaylistEntry *from, PlaylistEntry *to) {
  int pos = playback_queue.searchItem(from);
  if (pos < 0) return FALSE;
  int item_pos = getPositionForEntry(from);
  if (item_pos < 0) return FALSE;
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_QUEUE_DELETION, item_pos);
  playback_queue.removeByPos(pos);
  playback_queue.addItem(to, pos);
  ASSERT(getPositionForEntry(to) >= 0);
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_QUEUE_ADDITION, getPositionForEntry(to));
  return 1;
}

void PlaylistI::randomize() {
  // make a copy of the playlist entries
  PtrList<PlaylistEntry> tmp = entries;

  api->startCompleted(entries.getNumItems());
  // clear out the playlist
  entries.removeAll();

  // randomly refill it
  for (int i = 0; tmp.getNumItems(); i++) {
    int pick = Std::random32(tmp.getNumItems());
    PlaylistEntry *entry = tmp[pick];
    entries.addItem(entry);
    tmp.removeByPos(pick);
    api->incCompleted();
  }
  api->endCompleted();
  setModified();
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_RELOAD);
}

class strcmper {
public:
  static int compareItem(const char *p1, const char *p2) {
    return STRCMP(p1, p2);
  }
};

void PlaylistI::removeDups() {
  int ndups = 0;

  PtrListQuickSorted<char, strcmper> copy(entries.getNumItems());
  int i;
  for (i = 0; i < entries.getNumItems(); i++)
    copy.addItem(const_cast<char *>(entries[i]->getPlaystring()));
  const char *last = NULL;
  for (i = 0; i < copy.getNumItems(); i++) {
    if (last && STREQL(copy[i], last)) {
      removeByPlaystring(copy[i]);
      ndups ++;
    }
    last = copy[i];
  }
  if (ndups) setModified();
}

void PlaylistI::reverse() {
  if (getNumEntries() <= 1) return;
  int nitems = entries.getNumItems();
  PtrList<PlaylistEntry> copy(nitems);
  for (int i = 0; i < nitems; i++) {
    copy.addItem(entries[(nitems-i)-1]);
  }
  entries.copyFrom(&copy);

  setModified();
  // internal data was changed
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_RELOAD);
}

class SortEntryByString {
public:
  static int compareItem(PlaylistEntry *p1, PlaylistEntry *p2) {
    const char *v1 = p1->sortval ? ((String*)p1->sortval)->getValue() : "";
    const char *v2 = p2->sortval ? ((String*)p2->sortval)->getValue() : "";
    int r = STRCMP(v1, v2);
    if (r == 0) return CMP3(p1->prev_order, p2->prev_order);
    return r;
  }
};

class SortEntryByInt {
public:
  static int compareItem(PlaylistEntry *p1, PlaylistEntry *p2) {
    int v1 = (int)p1->sortval;
    int v2 = (int)p2->sortval;
    int r = CMP3(v1, v2);
    if (r == 0) return CMP3(p1->prev_order, p2->prev_order);
    return r;
  }
};

void PlaylistI::sortBy(const char *field) {
  if (getNumEntries() == 0) return;
  int datatype = api->metadb_getMetaDataType(field);
  if (datatype == MDT_NONE || datatype == MDT_ERROR) return;
  int n = getNumEntries();
  switch (datatype) {
    case MDT_STRINGZ: {
      char buf[4096];
      int i;
      for (i = 0; i < n; i++) {
        PlaylistEntry *ent = entries.q(i);
        int r = api->metadb_getMetaData(ent->getPlaystring(), field, buf, sizeof(buf), datatype);
        if (r <= 0) ent->sortval = NULL;
        else ent->sortval = (void*)new String(buf);
        ent->prev_order = i;
      }

      PtrListQuickSorted<PlaylistEntry, SortEntryByString> copy;
      copy.copyFrom(&entries);
      // autosort here
      entries.copyFrom(&copy);

      for (i = 0; i < n; i++) {
        PlaylistEntry *ent = entries.q(i);
        delete (String*)ent->sortval;
        ent->sortval = NULL;
      }
    }
    break;
    case MDT_TIMESTAMP:
    case MDT_TIME:
    case MDT_BOOLEAN:
    case MDT_INT: {
      int buf;
      for (int i = 0; i < n; i++) {
        PlaylistEntry *ent = entries.q(i);
        int r = api->metadb_getMetaData(ent->getPlaystring(), field, (char*)&buf, sizeof(int), datatype);
        if (r <= 0) ent->sortval = 0;
        else ent->sortval = (void*)buf;
        ent->prev_order = i;
      }

      PtrListQuickSorted<PlaylistEntry, SortEntryByInt> copy;
      copy.copyFrom(&entries);
      // autosort here
      entries.copyFrom(&copy);
    }
    break;
  }

  setModified();
  // internal data was changed
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_RELOAD);
}

void PlaylistI::lock(int _locked) {
  setDataAsInt("Locked", _locked);
}

int PlaylistI::isLocked() {
  return getDataAsInt("Locked") | getDataAsInt("UserLocked");
}

int PlaylistI::isModified() {
  return modified;
}

int PlaylistI::readFromFile(const char *filename, int enable_metadata, int enable_database_add) {
  ASSERT(filename != NULL);

  // strip down to base file name
  String basename = Std::filename(filename);
  char *p = const_cast<char *>(Std::extension(basename));
  if (p) *p = 0;
  else return -1;	// no extension, can't load it

  int c = 0;

  deleteAll();	// delete all entries to make room for incoming

//  api->main_enableInput(FALSE);
  _enable_metadata = enable_metadata;
  _enable_database_add = enable_database_add;

  addPlaylistItem(filename);

  _enable_metadata = TRUE;
  _enable_database_add = TRUE;

//  api->endCompleted();

  api->main_enableInput(TRUE);

  if (getPlaylistName() == NULL) setPlaylistName(basename);

  // it's not modified when it's been loaded
  setModified(0);

  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_RELOAD);

  return c;
}

int PlaylistI::writeToFile(const char *filename, int full_data) {
  ASSERT(filename != NULL);

  svc_playlistWriter *svc = PlaylistWriterEnum(filename).getNext();

  if (svc == NULL) return 0;

  svc->enableMetadata(full_data);

  if (svc->writePlaylist(filename, this, full_data, 0, MAX(getNumEntries()-1,0))<=0) {	// fall back

    if (!svc->beginWrite(filename, getNumEntries(), getPlaylistName())) {
      SvcEnum::release(svc);
      return 0;
    }
    for (int i = 0; i < getNumEntries(); i++) {
      PlaylistEntry *entry = enumEntry(i);
      svc->writeEntry(entry->getPlaystring());
    }

    svc->endWrite();
  }

  SvcEnum::release(svc);

  setModified(0);

  return 1;
}

void PlaylistI::setPlaybackCore(const char *_corename) {
  CoreHandle(corename).deregisterSequencer(plseq);
  corename = _corename;
}

int PlaylistI::startPlayback(int start) {
  CoreHandle maincore(corename);
  maincore.registerSequencer(plseq);
  plseq->setPlaylist(this);
  if (start) {
    maincore.stop();
    maincore.play();
  }
  return 1;
}

int PlaylistI::startPlaybackAtPos(int pos, int start) {
  CoreHandle maincore(corename);
  maincore.registerSequencer(plseq);
  setCurrent(pos);
  plseq->setPlaylist(this);
  if (start) {
    maincore.stop();
    maincore.play();
  }
  return 1;
}

void PlaylistI::setDontRandomizeFirst() {
  ASSERT(plseq != NULL);
  plseq->setDontRandomizeFirst();
}

void PlaylistI::setDontGetNextFromQueue() {
  plseq->setDontGetNextFromQueue();
}

int PlaylistI::setMetadata(const char *tag, const char *ptr, int ptrlen) {
  MemBlock<char> *mb = NULL;
  plmetadata.getItem(String(tag), &mb);
  if (mb == NULL) {// no record, make new one
    if (ptr != NULL && ptrlen > 0) {	// no new record if deleting
      mb = new MemBlock<char>(ptrlen, ptr);
      plmetadata.addItem(String(tag), mb);
    }
  } else {
    if (ptr == NULL || ptrlen <= 0) {	// delete record
      plmetadata.delItem(String(tag));
      delete mb; mb = NULL;
    } else {
      mb->setMemory(ptr, ptrlen);
    }
  }

  dependent_sendEvent(Playlist::depend_getClassGuid(), Playlist::Event_SETMETADATA, reinterpret_cast<int>(tag), (void *)ptr, ptrlen);

  return 1;
}

const char *PlaylistI::getMetadata(const char *tag, int *ptrlen) {
  MemBlock<char> *mb = NULL;
  plmetadata.getItem(String(tag), &mb);
  if (mb == NULL) return NULL;
  if (ptrlen != NULL) *ptrlen = mb->getSizeInBytes();
  return mb->getMemory();
}

const char *PlaylistI::enumMetadata(int n) {
  if (n < 0 || n >= plmetadata.getNumPairs()) return NULL;
  static String ret;
  ret = plmetadata.enumIndexByPos(n, String());
  return ret;
}


PlaylistEntry *PlaylistI::newPlaylistEntry(const char *playstring) {
  ASSERT(playstring != NULL);
  return plentry_freelist.newRecord(playstring);
}

void PlaylistI::delPlaylistEntry(PlaylistEntry *entry) {
  if (entry == NULL) return;
  onItemDel(entry);
  plentry_freelist.deleteRecord(entry);
}

void PlaylistI::deleteCachedInfo() {
  for (int i = 0; i < entries.getNumItems(); i++)
    entries[i]->delCachedInfo();
}

const PtrList<PlaylistEntry> *PlaylistI::getPtrList() {
  return &entries;
}

void PlaylistI::onItemAdd(PlaylistEntry *entry) {
  setModified();
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_ITEMS_ADDED);
}

void PlaylistI::onItemDel(PlaylistEntry *entry) {
  setModified();
  // delete from playback queue if it's in there
  if (playback_queue.getNumItems() != 0) {
    int pos = playback_queue.searchItem(entry);
    if (pos >= 0) {
      playback_queue.removeByPos(pos);
      // notify if queue empties
      if (playback_queue.getNumItems() == 0)
        dependent_sendEvent(Playlist::depend_getClassGuid(), Event_QUEUE_EMPTY);
      // repaint since numbers will change
      dependent_sendEvent(Playlist::depend_getClassGuid(), Event_REPAINT, -1);
    }
  }
  dependent_sendEvent(Playlist::depend_getClassGuid(), Event_ITEMS_DELETED);
}

void PlaylistI::setModified(int modify) {
  if (modify == modified) return;
  modified = modify;
}

#include "m3u.h"
#include "pls.h"
#include "bpl.h"
#include "asx.h"
#include "plhtml.h"

#include "../common/filename.h"
class DisableCmd : public ContextCmdT<Filename> {
public:
  DisableCmd() : ContextCmdT<Filename>() {
    addCommandById("Disable playback", DISABLE_PLAYBACK);
    checked = 0;
  }
  virtual int getChecked(DragItem *item, int n) {
    int nitems = item->getNumData();
    int non=0, noff=0;
    for (int i = 0; i < nitems; i++) {
      Filename *fn = static_cast<Filename*>(item->getDatum(i));
      int v = 0;
      api->metadb_getMetaData(fn->getFilename(), MT_DISABLED, (char *)&v, 4, MDT_BOOLEAN);
      if (v) non++;
      else noff++;
      if (non && noff) break;
    }
    if (non) {
      checked = 1;
      if (noff) checked++;
    }
    return checked;
  }
  virtual void onCommand(Filename* item, int n, int cmdid) {
    if (cmdid == DISABLE_PLAYBACK) {	// disable
      // set disabled status
      const char *ps = item->getFilename();
      int v = !checked;
      api->metadb_setMetaData(ps, MT_DISABLED, (const char *)&v, 4, MDT_BOOLEAN);
    }
  }
protected:
  enum { DISABLE_PLAYBACK };
private:
  int checked;
};

void PlaylistI::registerServices(WAComponentClient *wac) {
  wac->registerService(new ContextCmdCreator<PlaylistSortName>);
  wac->registerService(new ContextCmdCreator<PlaylistSortArtist>);
  wac->registerService(new ContextCmdCreator<PlaylistSortAlbum>);
  wac->registerService(new ContextCmdCreator<PlaylistSortFilename>);
  wac->registerService(new ContextCmdCreator<PlaylistSortLength>);
  wac->registerService(new ContextCmdCreator<PlaylistSortTrack>);
//later  wac->registerService(new ContextCmdCreator<PlaylistSortLastPlay>);
  wac->registerService(new ContextCmdCreator<PlaylistReverse>);
  wac->registerService(new ContextCmdCreator<PlaylistRandomize>);
  wac->registerService(new ContextCmdCreator<PlaylistSendTo>);
  wac->registerService(new ContextCmdCreator<PlaylistAdd>);

  wac->registerService(new ContextCmdCreator<PlaylistLoad>);
  wac->registerService(new ContextCmdCreator<PlaylistSaveAs>);
  wac->registerService(new ContextCmdCreator<PlaylistRemoveFromDir>);
  wac->registerService(new ContextCmdCreator<PlaylistRemoveDups>);
  wac->registerService(new ContextCmdCreator<PlaylistLock>);

  wac->registerService(new ContextCmdCreator<DisableCmd>);

  wac->registerService(new PlaylistWriterCreator<HtmlWriter>);
  wac->registerService(new PlaylistReaderCreator<HtmlLoader>);

  wac->registerService(new PlaylistReaderCreator<PlsLoader>);
  wac->registerService(new PlaylistWriterCreator<PlsWriter>);
  wac->registerService(new PlaylistReaderCreator<B4sLoader>);
  wac->registerService(new PlaylistReaderCreator<BplLoader>);
  wac->registerService(new PlaylistWriterCreator<B4sWriter>);
  wac->registerService(new PlaylistReaderCreator<M3uLoader>);
  wac->registerService(new PlaylistWriterCreator<M3uWriter>);
  wac->registerService(new PlaylistReaderCreator<AsxLoader>);

  wac->registerService(new PlaylistReaderCreator<QueryLoader>);
  wac->registerService(new PlaylistReaderCreator<DirLoader>);
}

#define CBCLASS PlaylistX
START_DISPATCH;
  CB(GETDEPENDENCYPTR, getDependencyPtr);
  CB(GETCFGITEM, getCfgItem);
  CB(GETPLAYLISTNAME, getPlaylistName);
  VCB(SETPLAYLISTNAME, setPlaylistName);
  CB(GETNUMENTRIES, getNumEntries);
  CB(ENUMENTRY, enumEntry);
  CB(ENUMITEM, enumItem);
  CB(GETPLAYSTRINGFROMENTRY, getPlayStringFromEntry);
  VCB(ENTRYENABLE, entryEnable);
  CB(ENTRYENABLED, entryEnabled);
  CB(GETHANDLE, getHandle);
  CB(GETPOSITIONFORENTRY, getPositionForEntry);
  CB(GETCACHEDNAMEFORENTRY, getCachedNameForEntry);
  CB(GETCACHEDLENGTHFORENTRY, getCachedLengthForEntry);
  VCB(RANDOMIZE, randomize);
  VCB(REMOVEDUPS, removeDups);
  VCB(REVERSE, reverse);
  VCB(SORTBY, sortBy);
  VCB(LOCK, lock);
  CB(ISLOCKED, isLocked);
  CB(ISMODIFIED, isModified);
  VCB(MARKNEXTFILE, markNextFile);
  VCB(UNMARKFILE, unmarkFile);
  CB(GETNEXTQUEUEDFILE, getNextQueuedFile);
  CB(GETQUEUEPOS, getQueuePos);
  CB(GETNUMQUEUED, getNumQueued);
  CB(REPLACEQUEUEENTRY, replaceQueueEntry);
  case ADDPLAYLISTITEM:
    if (nparam == 2) cb(&CBCLASS::addPlaylistItem2, retval, params);
    else cb(&CBCLASS::addPlaylistItem, retval, params);
  break;
  CB(REMOVEBYENTRY, removeByEntry);
  CB(REMOVEBYPOS, removeByPos);
  VCB(DELETEALL, deleteAll);
  VCB(DELETECACHEDINFO, deleteCachedInfo);
  CB(SETCURRENT, setCurrent);
  CB(GETCURRENT, getCurrent);
  CB(WRITETOFILE, writeToFile);
  CB(2900, old_readFromFile);
  CB(READFROMFILE, readFromFile);
  CB(STARTPLAYBACK, startPlaybackAtPos);
  CB(STARTPLAYBACK2, startPlayback);
  VCB(SETDONTRANDOMIZEFIRST, setDontRandomizeFirst);
  VCB(SETDONTGETNEXTFROMQUEUE, setDontGetNextFromQueue);
  VCB(ENSUREVISIBLE, ensureVisible);
  CB(SETMETADATA, setMetadata);
  CB(GETMETADATA, getMetadata);
  CB(ENUMMETADATA, enumMetadata);
END_DISPATCH;
#undef CBCLASS
