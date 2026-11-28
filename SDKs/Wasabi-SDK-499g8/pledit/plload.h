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
#ifndef _PLLOAD_H
#define _PLLOAD_H

#include <bfc/ptrlist.h>
#include <bfc/named.h>

#include "svc_playlist.h"

class Playstring;

// a helper class to define a playlist loader

class PlaylistLoader : public svc_playlistReaderI, public Named {
protected:
  PlaylistLoader();
  virtual ~PlaylistLoader();

protected:
  virtual const char *getExtension()=0;
  virtual int readPlaylist(const char *filename)=0;

  // no trailing slashes please
  void setPlaylistPath(const char *newpath);
  void setPlaylistPathFromFilename(const char *fn);

  virtual void enableDatabaseAdd(int enabled);

  virtual const char *getLabel();
  virtual int getNumEntries();
  virtual const char *enumEntry(int n);

  static void removeSlashes(char *buf);

  // handles relative pathing for you
  void addPlaylistEntry(const char *playstring, const char *initial_name=NULL);
  // handles relative pathing for you
  void applyMetaData(const char *playstring, const char *name, const char *data, int datalen, int datatype);

  static void trimWhitespace(char *str);	// trims in place
  static int needs_file(const char *str);
  static int needs_path(const char *str);

  // de-relativizes the playstring
  // uses static String* for storage, so don't try to keep * around
  // don't call this directly, addPlaylistItem() calls it for you
  static const char *adjustPlaystring(const char *str, const char *path);

private:
  PtrList<Playstring> entries;
  String plpath;
  int do_db_add;
};

#endif
