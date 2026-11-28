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
#ifndef _M3U_H
#define _M3U_H

#include "plload.h"
#include "../bfc/string.h"

class M3uLoader : public PlaylistLoader {
public:
  static const char *getServiceName() { return "M3U loader"; }

  virtual const char *getExtension() { return "m3u"; }
  virtual int readPlaylist(const char *filename);
};

class M3uWriter : public svc_playlistWriterI {
public:
  static const char *getServiceName() { return "M3U writer"; }

  M3uWriter();

  virtual const char *getExtension() { return "m3u"; }
  virtual const char *getDescription() { return "M3U playlist"; }

  virtual void enableMetadata(int enabled);

  virtual int beginWrite(const char *filename, int n, const char *label);
  virtual void writeEntry(const char *playstring);
  virtual void endWrite();

private:
  String filename;
  char path[WA_MAX_PATH];
  FILE *fp;
  int use_metadata;
};


#endif
