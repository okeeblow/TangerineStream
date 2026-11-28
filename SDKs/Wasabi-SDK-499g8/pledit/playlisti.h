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
#ifndef _PLAYLISTI_H
#define _PLAYLISTI_H

#include "playlist.h"
#include "plhand.h"

#include "../common/playstring.h"

#include "../bfc/depend.h"
#include "../bfc/freelist.h"
#include "../bfc/map.h"
#include "../bfc/named.h"
#include "../bfc/ptrlist.h"
#include "../bfc/string.h"

#include <bfc/attribs/cfgitemi.h>

// drag types
#define DD_PLENTRY "DD_PlaylistEntry v1 (private)"

class WAComponentClient;
class PlaylistSequencer;

class PlaylistEntry {
protected:
  friend class PlaylistI;
  friend class Freelist<PlaylistEntry>;
  PlaylistEntry(const char *playstring=NULL);
  ~PlaylistEntry();

public:
  const char *getPlaystring() { return playstring; }

  const char *getCachedName(int autoread=TRUE);
  int getCachedLength(int autoread=TRUE);
  int getCachedEnabled();
  void delCachedInfo();

  void setCurrent(int c) { current = !!c; }
  int getCurrent() const { return current; }

  void setWasCurrent(int c) { wascurrent = !!c; }
  int getWasCurrent() const { return wascurrent; }

  void setShuffleId(int s) { prev_shuffleid = shuffleid; shuffleid = s; }
  int getShuffleId() const { return shuffleid; }
  void revertShuffleId() { shuffleid = prev_shuffleid; prev_shuffleid = -1; }

  int getRenderMode() const { return rendermode; }

  void *sortval;
  int prev_order;

private:
  unsigned int current:1, wascurrent:1, enabled:1, user_enabled:1;
  int shuffleid, prev_shuffleid;
  Playstring playstring;
  const char *cached_name;
  int cached_length;
  int rendermode;
};

class PlaylistX : public Playlist {
public:
  virtual ~PlaylistX() { }

  virtual Dependent *getDependencyPtr()=0;
  virtual CfgItem *getCfgItem()=0;
  virtual const char *getPlaylistName()=0;
  virtual void setPlaylistName(const char *newname)=0;
  virtual int getNumEntries()=0;
  virtual PlaylistEntry *enumEntry(int which)=0;
  virtual const char *enumItem(int which)=0;
  virtual const char *getPlayStringFromEntry(PlaylistEntry *entry)=0;
  virtual void entryEnable(PlaylistEntry *entry, int enabled)=0;
  virtual int entryEnabled(PlaylistEntry *entry)=0;
  virtual PlaylistHandle getHandle()=0;
  virtual int getPositionForEntry(PlaylistEntry *entry)=0;
  virtual const char *getCachedNameForEntry(PlaylistEntry *entry, int autoread)=0;
  virtual int getCachedLengthForEntry(PlaylistEntry *entry, int autoread)=0;
  virtual void randomize()=0;
  virtual void removeDups()=0;
  virtual void reverse()=0;
  virtual void sortBy(const char *field)=0;
  virtual void lock(int locked)=0;
  virtual int isLocked()=0;
  virtual int isModified()=0;
  virtual void markNextFile(int pos, int insert_at_front=FALSE)=0;
  virtual void unmarkFile(int pos)=0;
  virtual const char *getNextQueuedFile(int *pos)=0;
  virtual int getQueuePos(int pos)=0;
  virtual int getNumQueued()=0;
  virtual int replaceQueueEntry(PlaylistEntry *from, PlaylistEntry *to)=0;
  virtual int addPlaylistItem(const char *playstring, int after=APPEND, int expand=TRUE)=0;
  virtual int addPlaylistItem2(const char *playstring, int after=APPEND)=0;//back compat
  virtual int removeByEntry(PlaylistEntry *entry)=0;
  virtual int removeByPos(int nitem)=0;
  virtual int removeByPlaystring(const char *playstring)=0;
  virtual void deleteAll()=0;
  virtual void ensureVisible(int pos)=0;
  virtual void deleteCachedInfo()=0;
  virtual int setCurrent(int pos)=0;
  virtual int getCurrent()=0;
  virtual int writeToFile(const char *filename, int full_data=FALSE)=0;
  int old_readFromFile(const char *filename) {
    return readFromFile(filename, TRUE, TRUE);
  }
  virtual int readFromFile(const char *filename, int enable_metadata, int enable_database_add)=0;
  virtual void setPlaybackCore(const char *corename)=0;
  virtual int startPlayback(int start)=0;
  virtual int startPlaybackAtPos(int pos, int start)=0;
  virtual void setDontRandomizeFirst()=0;
  virtual void setDontGetNextFromQueue()=0;
  virtual int setMetadata(const char *tag, const char *ptr, int ptrlen)=0;
  virtual const char *getMetadata(const char *tag, int *ptrlen=NULL)=0;
  virtual const char *enumMetadata(int n)=0;

private:
  RECVS_DISPATCH;
};

class PlaylistI : public PlaylistX, public CfgItemI /*public DependentI, public Named*/ {
public:
  PlaylistI(/*CUTPlaylistHandle handle=INVALID_PLAYLIST_HANDLE, Playlist *cloner=NULL*/const char *loadfrom=NULL, int full_metadata=FALSE);
  virtual ~PlaylistI();

  void setPlaylistHandle(PlaylistHandle h) { handle = h; }

  Dependent *getDependencyPtr() { return this; }//
  CfgItem *getCfgItem() { return this; }//
  virtual int cfgitem_usePrivateStorage() { return 1; }

// allows casting from Dependent to Playlist
  virtual void *dependent_getInterface(const GUID *classguid);

  const char *getPlaylistName(); //
  void setPlaylistName(const char *newname); //

  virtual void cfgitem_onSetName();

  int getNumEntries();//
  PlaylistEntry *enumEntry(int which);//
  const char *enumItem(int which);//
  const char *getPlayStringFromEntry(PlaylistEntry *entry);//
  void entryEnable(PlaylistEntry *entry, int enabled);//
  int entryEnabled(PlaylistEntry *entry);//

  PlaylistHandle getHandle() { return handle; }//

  int getPositionForEntry(PlaylistEntry *entry);//
  const char *getCachedNameForEntry(PlaylistEntry *entry, int autoread);//
  int getCachedLengthForEntry(PlaylistEntry *entry, int autoread);//

  //sorting
  void randomize();//
  void removeDups();//
  void reverse();//
  void sortBy(const char *field);//

  // modified?
  void lock(int locked);
  int isLocked();
  int isModified();

  void markNextFile(int pos, int insert_at_front=FALSE);
  void unmarkFile(int pos);
  const char *getNextQueuedFile(int *pos);
  int getQueuePos(int pos);//
  int getNumQueued();//
  int replaceQueueEntry(PlaylistEntry *from, PlaylistEntry *to);//

  int addPlaylistItem2(const char *playstring, int after=APPEND);//
  int addPlaylistItem(const char *playstring, int after=APPEND, int expand=TRUE);//
private:
  int _addPlaylistItem(const char *playstring, int &after, int expand, int recursionlevel=0);
  int _enable_metadata, _enable_database_add;

public:
  int removeByEntry(PlaylistEntry *entry);//
  int removeByPos(int nitem);//
  int removeByPlaystring(const char *playstring);	// removes first found
  void deleteAll();//

  void ensureVisible(int pos);

  void deleteCachedInfo();//

  int cloneFrom(Playlist *pl);

  int setCurrent(int pos);//
  int getCurrent();//

  virtual void onNextFile();	// from sequencer

  // i/o
  int writeToFile(const char *filename, int enable_metadata=FALSE);//
  int readFromFile(const char *filename, int enable_metadata, int enable_database_add);//

  void setPlaybackCore(const char *corename);//
  int startPlayback(int start);//
  int startPlaybackAtPos(int pos, int start);//
  void setDontRandomizeFirst();//
  void setDontGetNextFromQueue();//

  // metadata
  int setMetadata(const char *tag, const char *ptr, int ptrlen);//
  const char *getMetadata(const char *tag, int *ptrlen=NULL);//
  const char *enumMetadata(int n);

  // playlist editor/dir data

  virtual void setLastPos(int lp) { last_pos = lp; }
  virtual int getLastPos() { return last_pos; }

  PlaylistEntry *newPlaylistEntry(const char *playstring);
  void delPlaylistEntry(PlaylistEntry *entry);

  static void registerServices(WAComponentClient *wac);

protected:
friend class PlaylistSequencer;
  const PtrList<PlaylistEntry> *getPtrList();

  void onItemAdd(PlaylistEntry *entry);
  void onItemDel(PlaylistEntry *entry);
  void setModified(int modify=TRUE);

private:
  PlaylistHandle handle;
  void expandItem(int pos);
  PtrList<PlaylistEntry> entries;
  int last_pos;

  int modified;

  PtrList<PlaylistEntry> playback_queue;

  Map<String, MemBlock<char>*> plmetadata;

  PlaylistSequencer *plseq;
  String corename;
};

#endif
