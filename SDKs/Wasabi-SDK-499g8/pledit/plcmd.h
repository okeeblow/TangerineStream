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
#ifndef _PLCMD_H
#define _PLCMD_H

#include "playlist.h"
#include "../common/contextcmd.h"
#include "../common/metatags.h"

class svc_fileSelector;

class PlaylistCmdM : public ContextCmdSingleMethod<Playlist> {
protected:
  typedef void (Playlist::*cbfn)();
  PlaylistCmdM(const char *text, cbfn fn=NULL, const char *menu_path=NULL) :
    ContextCmdSingleMethod<Playlist>(text, fn, menu_path) {}
};

class PlaylistCmd : public ContextCmdSingle<Playlist> {
protected:
  PlaylistCmd(const char *text, const char *menu_path=NULL) :
    ContextCmdSingle<Playlist>(text, NULL, menu_path) {}

  virtual int getEnabled(DragItem *item, int n);
};

class PlCmdSort : public PlaylistCmd {
public:
  PlCmdSort(const char *field, const char *name=NULL);
  virtual void onCommand(Playlist *item, int n, int cmdid);
private:
  String fieldname;
};

class PlaylistSortName : public PlCmdSort {
public:
  PlaylistSortName() : PlCmdSort(MT_NAME) {}
};

class PlaylistSortArtist : public PlCmdSort {
public:
  PlaylistSortArtist() : PlCmdSort(MT_ARTIST) {}
};

class PlaylistSortAlbum : public PlCmdSort {
public:
  PlaylistSortAlbum() : PlCmdSort(MT_ALBUM) {}
};

class PlaylistSortFilename : public PlCmdSort {
public:
  PlaylistSortFilename() : PlCmdSort(MT_PLAYSTRING, "path filename") {}
};

class PlaylistSortLength : public PlCmdSort {
public:
  PlaylistSortLength() : PlCmdSort(MT_LENGTH) {}
};

class PlaylistSortTrack : public PlCmdSort {
public:
  PlaylistSortTrack() : PlCmdSort(MT_TRACK) {}
};


class PlaylistSortLastPlay : public PlCmdSort {
public:
  PlaylistSortLastPlay() : PlCmdSort(MT_LASTPLAYED, "last play time") {}
};


class PlaylistRandomize : public PlaylistCmdM {
public:
  PlaylistRandomize() : PlaylistCmdM("Randomize order", Playlist::randomize, "Sort") {}
};

class PlaylistReverse : public PlaylistCmdM {
public:
  PlaylistReverse() : PlaylistCmdM("Reverse order", Playlist::reverse, "Sort") {}
};

class PlaylistLoad : public PlaylistCmd {
public:
  PlaylistLoad() : PlaylistCmd("Load playlist", "Playlist") {}
protected:
  virtual void onCommand(Playlist *item, int n, int cmdid);
};

class PlaylistSaveAs : public PlaylistCmd {
public:
  PlaylistSaveAs() : PlaylistCmd("Save playlist as...", "Playlist") {}

protected:
  virtual void onCommand(Playlist *item, int n, int cmdid);
};

class PlaylistRemoveFromDir : public PlaylistCmd {
public:
  PlaylistRemoveFromDir() : PlaylistCmd("Unload playlist", "Playlist") {}

protected:
  virtual void onCommand(Playlist *item, int n, int cmdid);
};

class PlaylistRemoveDups : public PlaylistCmdM {
public:
  PlaylistRemoveDups() : PlaylistCmdM("Remove duplicate entries", Playlist::removeDups) {}
};

class PlaylistLock : public PlaylistCmd {
public:
  PlaylistLock() : PlaylistCmd("Lock playlist", "Playlist") {}
  virtual int getEnabled(DragItem *item, int n);
  virtual int getChecked(DragItem *item, int n);
  virtual void onCommand(Playlist *item, int n, int cmdid);
};

class Filename;

class PlaylistSendTo : public ContextCmdT<Filename> {
public:
  PlaylistSendTo();
protected:
  virtual void populateCommands();
  virtual void onCommand(Filename *item, int n, int cmdid);
private:
  Playlist *addpl;
};

class PlaylistAdd : public ContextCmdT<Playlist> {
public:
  PlaylistAdd();
  virtual ~PlaylistAdd();

  virtual void populateCommands();
  virtual void onCommand(Playlist *item, int n, int cmdid);

private:
  PtrList<svc_fileSelector> fslist;
};

#endif
