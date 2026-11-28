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
#ifndef _PLDIRSVC_H
#define _PLDIRSVC_H

#include "svc_pldir.h"
#include "../studio/services/svc_droptarget.h"

#include "../bfc/ptrlist.h"
#include "../bfc/depend.h"
#include "../bfc/drag.h"
#include "../studio/wac.h"
#include "../bfc/timerclient.h"

#include "../studio/runlevelcb.h"

class PlaylistI;
class PlaylistSlot;

class PlDirSvc : public svc_plDirI, RunlevelCallbackI, public LoadableResource, public DragInterfaceI, TimerClientI {
public:
  PlDirSvc();
  virtual ~PlDirSvc();

  // LoadableResource
  virtual int deleteOnShutdown() { return FALSE; }
  virtual void onRegisterServices();
  virtual void onDeregisterServices();

  static const char *getServiceName() { return "Playlist Directory"; }

  virtual int getNumPlaylists(int skip_hidden=TRUE);
  virtual PlaylistHandle enumPlaylist(int n, int skip_hidden=TRUE);
  virtual Playlist *getPlaylist(PlaylistHandle handle);
  virtual void ungetPlaylist(Playlist *pl);

  virtual const char *getPlaylistLabel(PlaylistHandle handle);
  virtual int setPlaylistLabel(PlaylistHandle handle, const char *newlabel);

  virtual PlaylistHandle insertPlaylist(const char *filename=NULL, const char *label=NULL, const char *path=NULL, int hidden=FALSE);
  virtual int removePlaylist(PlaylistHandle handle);
  virtual int setAutoSave(PlaylistHandle handle, int autosave);
  virtual void clearAll();

  virtual int setHidden(PlaylistHandle handle, int hidden);
  virtual int getHidden(PlaylistHandle handle);

// RunlevelCallbackI
  virtual void runlevelcb_onAppRunning();
  virtual void runlevelcb_onShutdown();

// DragInterfaceI
  virtual int dragDrop(RootWnd *sourceWnd, int x, int y);

// TimerClientI
  virtual Dependent *timerclient_getDependencyPtr() { return this; }
  virtual int timerclient_onDeferredCallback(int param1, int param2);
  virtual void timerclient_timerCallback(int id);

  void processCmdLine(const char *cmdline, int seqno);

  virtual void setCurrentlyOpen(PlaylistHandle handle);
  virtual PlaylistHandle getCurrentlyOpen();
  virtual PlaylistHandle getCurrentlyPlaying();

  virtual const char *getPlaystringFromEntry(PlaylistEntry *entry);

  String makeNewPlName();

private:
  PlaylistSlot *slotFromHandle(PlaylistHandle handle);
  PlaylistHandle addPlaylist(PlaylistSlot *pl);
  void delPlaylist(PlaylistSlot *pl);	// also deletes
  static String inventFileName();
  PtrList<PlaylistSlot> directory;
  PlaylistHandle currentlyopen;

  PtrList<String> files;	// accumulator of cmdline files
  int force_enqueue;

  int auto_start;
};

#endif
