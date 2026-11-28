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

#include "m3u.h"
#include "../bfc/std.h"
#include "../common/metatags.h"
#include "../studio/api.h"

int makeRelative(char *filename, const char *path);

int M3uLoader::readPlaylist(const char *filename) {
  FILE *fp = FOPEN(filename, "rt");
  if (fp == NULL) return -1;

  // extract path
  setPlaylistPathFromFilename(filename);

  char buf[WA_MAX_PATH];
  int is_ext = 0;
  int ext_len=-1;
  char ext_name[WA_MAX_PATH]="";
  int c = 0;
  while (FGETS(buf, sizeof(buf)-1, fp)) {
    buf[WA_MAX_PATH-1] = 0;
    trimWhitespace(buf); // clean off carriage returns and whatnot

    if (STREQL(buf, "#EXTM3U")) is_ext = 1;

    if (is_ext && !STRNNCMP(buf, "#EXTINF:")) {
      ext_len = ATOI(buf)+8; if (ext_len == 0) ext_len = -1;
      ext_name[0] = 0;
      const char *extnc = STRCHR(buf, ',');
      if (extnc != 0) STRNCPY(ext_name, extnc+1, sizeof(ext_name));
      continue;
    }
    if (*buf == 0 || *buf == '#') continue;
    // build up the playlist
    addPlaylistEntry(buf, ext_name);
    // reset extended name for next time around
    ext_name[0] = 0;
    c++;
  }

  FCLOSE(fp);

  return c;
}

M3uWriter::M3uWriter() : fp(NULL), use_metadata(TRUE) {
}

void M3uWriter::enableMetadata(int enabled) {
  use_metadata = enabled;
}

int M3uWriter::beginWrite(const char *_filename, int n, const char *label) {
  filename = _filename;
  if (filename.isempty()) return 0;
  fp = FOPEN(filename, "wt");
  if (!fp) return 0;

  if (use_metadata) FPRINTF(fp, "#EXTM3U\n");

  STRNCPY(path, filename, WA_MAX_PATH);
  path[WA_MAX_PATH-1] = 0;
  const char *p = Std::scanstr_back(path, DIRCHARSTR, NULL);
  if (p == NULL) *path = 0;
  else {
    const_cast<char *>(p)[1] = 0;
  }

  return 1;
}

void M3uWriter::writeEntry(const char *playstring) {
  if (use_metadata) {
    int len = api->metadb_getLength(playstring);
    if (len > 0) len /= 1000;
    char buf[WA_MAX_PATH]="";
    int r = api->metadb_getMetaData(playstring, MT_NAME, buf, sizeof buf);
    if (r) FPRINTF(fp, "#EXTINF:%d,%s\n", len, buf);
  }
  if (!STRNNCMP(playstring, "file:")) {
    String nfn = playstring+STRLEN("file:");
    makeRelative(nfn.getNonConstVal(), path);
    FPRINTF(fp, "%s\n", nfn.getValue());
  } else {
    FPRINTF(fp, "%s\n", playstring);
  }
}

void M3uWriter::endWrite() {
  FCLOSE(fp);
  fp = NULL;
}

// copied from wa2, but still not 100% sure it matches
int makeRelative(char *filename, const char *path) {

  // no path = no relativity
  if (*path == 0) return 0;

  // perfect match = yay
  int pathlen = STRLEN(path);
  if (pathlen < STRLEN(filename) && !STRNINCMP(filename, path)) {
    STRCPY(filename, filename + pathlen);
    return 1;
  }

  if (!STRNINCMP(filename, "file:")) {
    // try for just the drive letter (I think)
    if (path[1] == ':' && path[2] == DIRCHAR) {
      String path2(path);
      path2.trunc(2);
      if (path2.len() < STRLEN(filename) && !STRNINCMP(filename,path2)) {
        STRCPY(filename, filename+path2.len());
      }
      return 1;
    }
  }

  // otherwise fuckit
  return 0;
}
