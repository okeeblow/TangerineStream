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

#include "plslot.h"
#include "playlisti.h"
#include <bfc/attribs/attribs.h>

static PlaylistHandle handle_gen=1;

PlaylistSlot::PlaylistSlot(const char *_filename, const char *_save, const char *_label, int hide) :
  orig_filename(_filename), save_filename(_save), label(_label),
  pl(NULL), handle(handle_gen++), autosave(TRUE), hidden(hide) {
  if (label.isempty()) {
    const char *fn = Std::filename(_filename);
    label = fn;
    if (label.isempty()) label = "[null]";
  }
}

PlaylistSlot::~PlaylistSlot() {
  delete pl;
}

PlaylistHandle PlaylistSlot::getHandle() {
  return handle;
}

Playlist *PlaylistSlot::getPlaylist() {
  if (pl == NULL) {
    pl = new PlaylistI;
    pl->setPlaylistHandle(getHandle());
    if (!orig_filename.isempty()) pl->readFromFile(orig_filename, FALSE, FALSE);
    pl->setName(getLabel());
  }
  return pl;
}

void PlaylistSlot::save() {
  if (pl != NULL) {
    if (pl->isModified()) {
OutputDebugString(StringPrintf("writing playlist: %s\n", save_filename));
      pl->writeToFile(save_filename);
    } else {
      OutputDebugString(StringPrintf("not writing unmodified playlist: %s\n", save_filename));
    }
  }
}

void PlaylistSlot::setLabel(const char *_label) {
  label = _label;
  if (pl != NULL) pl->setPlaylistName(label);
}

const char *PlaylistSlot::getLabel() {
  return label;
}

const char *PlaylistSlot::getSaveFileName() {
  return save_filename;
}

void PlaylistSlot::setAutoSave(int as) {
  autosave = !!as;
}

int PlaylistSlot::getAutoSave() {
  return autosave;
}

void PlaylistSlot::setHidden(int h) {
  hidden = h;
}

int PlaylistSlot::getHidden() {
  return hidden;
}
