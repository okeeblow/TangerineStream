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

#include "pls.h"

#include "../studio/api.h"
#include "../common/metatags.h"

#define PLS_VERSION 2

int PlsLoader::readPlaylist(const char *filename) {
  FILE *fp = FOPEN(filename, "rt");
  if (fp == NULL) return -1;

  setPlaylistPathFromFilename(filename);

  char ext_name[WA_MAX_PATH]="";
  char curFile[WA_MAX_PATH]="";
  char buf[WA_MAX_PATH/2];
  int ncode=0;
  int c = 0;

  while (FGETS(buf, sizeof(buf)-1, fp)) {
    // clean the string
    buf[sizeof(buf)-1] = 0;
    trimWhitespace(buf);

    if (!STRNNCMP(buf, "File")) {
      if (curFile[0]) {
        addPlaylistEntry(curFile, ext_name);
        c++;
        curFile[0]=0;
        ext_name[0]=0;
      }

      char *f=STRSTR(buf,"=")+1;
//CUT      if (f != NULL) {
//CUT        adjustString(f, curFile, path);
//CUT      }
      if (f != NULL) STRNCPY(curFile, f, sizeof(curFile)-2);
    } else if (!STRNNCMP(buf, "Title")) {
      char *t=STRSTR(buf,"=")+1;
      STRCPY(ext_name,t);
    } else if (!STRNNCMP(buf, "PlaylistName=")) {
      setName(buf+STRLEN("PlaylistName="));
    }
  }

  if (*curFile != '\0') {
    addPlaylistEntry(curFile);
    c++;
  }

  FCLOSE(fp);

  return c;
}

PlsWriter::PlsWriter() : fp(NULL), num_entries(0), enable_metadata(FALSE) {
}

void PlsWriter::enableMetadata(int enabled) {
  enable_metadata = enabled;
}

int PlsWriter::beginWrite(const char *filename, int n, const char *label) {
  fp = FOPEN(filename, "wt");
  FPRINTF(fp, "[playlist]\n");

  String labelo = label;
  if (!labelo.isempty()) FPRINTF(fp, "PlaylistName=%s\n", labelo.getValue());

  return 1;
}

void PlsWriter::writeEntry(const char *playstring) {
  // Write playstring
  if (!STRNINCMP(playstring, "file:")) playstring += 5;
  num_entries ++;
  FPRINTF(fp, "File%d=%s\n", num_entries, playstring);

  if (enable_metadata) {
    // Get length and title from the metadb
    int len = api->metadb_getLength(playstring);
    if (len > 0) len /= 1000;
    char buf[WA_MAX_PATH]="";
    int r = api->metadb_getMetaData(playstring, MT_NAME, buf, sizeof buf);
  
    // Write Title and Length
    if (r) FPRINTF(fp, "Title%d=%s\n", num_entries, buf);
    if (len > 0) 
      FPRINTF(fp, "Length%d=%d\n", num_entries, len);
    else
      FPRINTF(fp, "Length%d=-1\n", num_entries);
  }
}

void PlsWriter::endWrite() {
  if (PLS_VERSION > 1) {
    FPRINTF(fp, "NumberOfEntries=%d\n", num_entries);
    FPRINTF(fp, "Version=%d\n", PLS_VERSION);
  }
  FCLOSE(fp); fp = NULL;
}
