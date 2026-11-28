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
#ifndef _PLSEQ_H
#define _PLSEQ_H

#include "playlist.h"
#include <core/sequence.h>
#include <bfc/depview.h>

#define PLAYLISTSEQUENCER_PARENT ListSequencer
class PlaylistSequencer : public ListSequencer, public DependentViewerTPtr<Playlist> {
public:
  PlaylistSequencer();
  virtual ~PlaylistSequencer();

  int setPlaylist(Playlist *newpl);
  Playlist *getPlaylist();
  void setDontRandomizeFirst();
  void setDontGetNextFromQueue();

  virtual const char *getNextPlayItem();
  virtual int getCurrentPlaybackNumber();
  virtual int onNextFile();	// right when we transition

  virtual int rewind();
  virtual int forward();

  virtual int onDeregister();

  virtual int onStarted();
  virtual int onStopped();

protected:
  virtual int getNumEntries();
  virtual int getCurrent();
  virtual int setCurrent(int pos);
  virtual const char *enumItem(int pos);
  virtual int loop();

  virtual int viewer_onItemDeleted(Playlist *item);

private:
  PlaylistEntry *nextPlayItem();
  Playlist *playlist;
  int dont_randomize_first;
  int dont_get_next_from_queue;
  int rewinding, forwarding;
  int started;
};

#endif
