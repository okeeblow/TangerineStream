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
#include <math.h>

#include "plseq.h"
#include "pledit.h"
#include "playlisti.h"

#include "../studio/assert.h"
#include "../studio/api.h"

#include "../bfc/std.h"
#include "../common/metatags.h"

#include <bfc/attribs/attribs.h>

PlaylistSequencer::PlaylistSequencer() {
  playlist = NULL;
  dont_randomize_first = 1;
  dont_get_next_from_queue = 0;
  rewinding = forwarding = 0;
  started = 0;
}

PlaylistSequencer::~PlaylistSequencer() { }

int PlaylistSequencer::setPlaylist(Playlist *newpl) {

  viewer_delAllItems();
  viewer_addViewItem(newpl);

  playlist = newpl;

  return 1;
}

void PlaylistSequencer::setDontRandomizeFirst() {
#if defined(_DEBUG)
OutputDebugString(StringPrintf("DONTRANDO: was %d\n", dont_randomize_first));
#endif

  dont_randomize_first = 1;

#if defined(_DEBUG)
OutputDebugString(StringPrintf("DONTRANDO: --> %d\n", dont_randomize_first));
#endif
}

void PlaylistSequencer::setDontGetNextFromQueue() {
  dont_get_next_from_queue = 1;
}

Playlist *PlaylistSequencer::getPlaylist() {
  return playlist;
}

const char *PlaylistSequencer::getNextPlayItem() {
  extern _int repeat;
  if ((repeat == 3) && started || playlist == NULL) return "stop dammit";// stop on each or no playlist
//CUT  if (repeat != 2 && repeat != 3) playlist->onGetNextFile();	// repeat 1
  int was_dont_randomize_first = dont_randomize_first;
  PlaylistEntry *ret = NULL;
  for (int i = 0; i < playlist->getNumEntries(); i++) {
    ret = nextPlayItem();
    if (ret == NULL) break;
    int v = playlist->entryEnabled(ret);
    if (v) {
      int v2 = 0;
      api->metadb_getMetaData(ret->getPlaystring(), MT_DISABLED, (char*)&v2, sizeof(int), MDT_BOOLEAN);
      if (!v2) break;
    }
    if (repeat == 2 || repeat == 3) break;  // repeat 1 and stop on each
    forward();
  }
OutputDebugString("DONTRANDO == 0 $1");
  dont_randomize_first = 0;
  if (ret == NULL)
    return NULL;
  else
    return ret->getPlaystring();
#if 0//FUCKO
  extern _int nofadesequential;
  if (nofadesequential) {
    PlayItem *cur = api->core_getCurrent(0);
    int crossfadeto=TRUE;
    char albumr[WA_MAX_PATH]="";
    char albumc[WA_MAX_PATH]="";
    if (api->metadb_getMetaData(ret, MT_ALBUM, albumr, sizeof albumr, MDT_STRINGZ) > 0) {
      api->metadb_getMetaData(cur, MT_ALBUM, albumc, sizeof albumc, MDT_STRINGZ);
    }
    if (STRCASEEQL(albumr, albumc)) {
      char track1[3]="", track2[3]="";
      api->metadb_getMetaData(ret, MT_TRACK, track1, sizeof track1, MDT_STRINGZ);
      api->metadb_getMetaData(cur, MT_TRACK, track2, sizeof track2, MDT_STRINGZ);
      int t1 = ATOI(track1), t2 = ATOI(track2);
      if (t1 != 0 && t2 != 0 && t2 == t1 - 1) crossfadeto = FALSE;
    }
GUID baseGUID = 
{ 0x3cbd4483, 0xdc44, 0x11d3, { 0xb6, 0x8, 0x0, 0x0, 0x86, 0x34, 0x8, 0x85 } };
    api->cmd_sendCommand(baseGUID, "crossfade_enable", crossfadeto);
  }
#endif
}

int PlaylistSequencer::getCurrentPlaybackNumber() {
  if(!playlist) return 0;
  int pos = playlist->getCurrent();
  if (pos < 0) pos = 0;
  return pos;
}

int PlaylistSequencer::onNextFile() { // right when we transition
  PLAYLISTSEQUENCER_PARENT::onNextFile();
  playlist->onNextFile();
  return 1;
}

int PlaylistSequencer::rewind() {
  rewinding = 1, forwarding = 0;
  return PLAYLISTSEQUENCER_PARENT::rewind();
}

int PlaylistSequencer::forward() {
  rewinding = 0, forwarding = 1;
  return PLAYLISTSEQUENCER_PARENT::forward();
}

int PlaylistSequencer::onDeregister() {
  PLAYLISTSEQUENCER_PARENT::onDeregister();

  return 1;
}

int PlaylistSequencer::onStarted() {
  started=1;
  return 1;
}

int PlaylistSequencer::onStopped() {
  started = 0;
  setDontRandomizeFirst();
  return 1;
}

int PlaylistSequencer::getNumEntries() {
  ASSERT(playlist != NULL);
  return playlist->getNumEntries();
}

int PlaylistSequencer::getCurrent() {
  if (playlist == NULL) return -1;
  ASSERT(playlist != NULL);
  return playlist->getCurrent();
}

const char *PlaylistSequencer::enumItem(int pos) {
  ASSERT(playlist != NULL);
  return playlist->enumItem(pos);
}

int PlaylistSequencer::setCurrent(int cur) {
  ASSERT(playlist != NULL);
  return playlist->setCurrent(cur);
}

int PlaylistSequencer::loop() {
  extern _bool shuffle;
  extern _int repeat;
  return (repeat==1 || repeat==2) || shuffle;
}

int PlaylistSequencer::viewer_onItemDeleted(Playlist *item) {
  ASSERT(item == playlist);
  playlist = NULL;
  return 1;
}

class CompareEntry {
public:
  static int compareItem(PlaylistEntry *p1, PlaylistEntry* p2) {
    int ret = CMP3(p1->getShuffleId(), p2->getShuffleId());
    if (ret == 0) return CMP3(p1, p2);
    return ret;
  }
};

//static void updateShuffle(PlaylistEntry *entry) {
//  int shuffleid = api->getIntPrivate("shuffleid", 0);
//  entry->setShuffleId(++shuffleid);
//  api->setIntPrivate("shuffleid", shuffleid);
//}

PlaylistEntry *PlaylistSequencer::nextPlayItem() {
  extern _int repeat;
  ASSERT(playlist != NULL);

  int nentries = playlist->getNumEntries();
  // if no entries, bail
  if (nentries <= 0) return NULL;

  PlaylistEntry *entry = NULL;

  // build the shuffle ordered list
  PtrListQuickSorted<PlaylistEntry, CompareEntry> list;
  list.setAutoSort(false);
  list.copyFrom(static_cast<PlaylistI*>(playlist)->getPtrList());
  list.sort(true);

//  foreach(list)
//    OutputDebugString(StringPrintf("%d %s %d\n", foreach_index+1, list.getfor()->getPlaystring(), list.getfor()->getShuffleId()));
//  endfor

  int highest = list.getLast()->getShuffleId();

  int pos;
  const char *queued = NULL;
  if (!dont_get_next_from_queue) queued = playlist->getNextQueuedFile(&pos);
  dont_get_next_from_queue = 0;
  if (queued != NULL) {
    setCurrent(pos);
    playlist->enumEntry(pos)->setShuffleId(highest+1);
    return playlist->enumEntry(pos);
  }

  extern _bool shuffle;

  int do_shuffle = shuffle;

  if (dont_randomize_first) do_shuffle = 0;

  // freshly loaded
  if (playlist->getCurrent() == -1) {
    OutputDebugString("DONTRANDO == 0 #2");
    dont_randomize_first = 0;
  }
//CUT  if (playlist->getCurrent() != -1) dont_randomize_first = 1;

  if (do_shuffle) {
    extern _int shuffle_randomize;
    int r = shuffle_randomize;
    if (r == 0 || (r != 100 && Std::random(100) > r)) do_shuffle = 0;
  }

  // if repeat is off, shuffle is on,  but only 1 item in the playlist...don't get next 
  if (repeat == 0 && do_shuffle && nentries == 1) {
    return NULL;	
  }

  // if < 3 entries, don't waste time shuffling
  if (do_shuffle && nentries >= 3) {

    int ni = list.getNumItems();

    if (!rewinding) {	// grab a new one
      extern _int shuffle_window;
      int fni = (int)floor((((float)ni * shuffle_window) / 100.0f)+.5f);
      // if the window is 0 items pick the lowest one
      int pick = (fni > 0) ? Std::random32(fni) : 0;
      ASSERT(pick <= list.getNumItems());
      entry = list[pick];
    } else {
      PlaylistEntry *lastentry = list.enumItem(list.getNumItems()-1);
      entry = list.enumItem(list.getNumItems()-2);
      if (entry->getShuffleId() != -1) {
        lastentry->revertShuffleId();
      } else {
        entry = lastentry;
      }
    }
    ASSERT(entry != NULL);

    rewinding = forwarding = 0;

  } else {
    pos = getCurrent();
    if (pos < 0) pos = 0;	// start at 0 if none
    entry = playlist->enumEntry(pos);
  }
  ASSERT(entry != NULL);

  entry->setShuffleId(highest+1);
  setCurrent(playlist->getPositionForEntry(entry));

  return entry;
}
