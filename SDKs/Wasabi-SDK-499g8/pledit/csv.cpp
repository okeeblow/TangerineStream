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
#include "csv.h"
#include "playlist.h"

#include "../bfc/std.h"
#include "../bfc/string.h"
#include "../common/metatags.h"

#include "../studio/api.h"

static const char * CSV_HEADER    = "file,title,length\n";
static const char * FILE_PREFIX   = "file:";
static const int FILE_PREFIX_LEN  = 5;

CsvWriter::CsvWriter() {
  csvFile = NULL;
}

CsvWriter::~CsvWriter() {
  if(csvFile != NULL) {
    FCLOSE(csvFile);
    csvFile = NULL;
  }
}

int CsvWriter::writePlaylist(const char *filename, Playlist *pl, int full_data, int first, int last) {
  String playstring;
  String info;

  PlaylistEntry *entry = NULL;
  
  if((csvFile = FOPEN(filename, "w+")) == NULL) return 0;

  FWRITE(CSV_HEADER, STRLEN(CSV_HEADER), 1, csvFile);

  for(int i=first; i < last; i++) {
    entry = pl->enumEntry(i);
    playstring = pl->getPlayStringFromEntry(entry);

    if(!STRNICMP(playstring, FILE_PREFIX, FILE_PREFIX_LEN))
      playstring = playstring.rSplit(FILE_PREFIX_LEN);

    info = StringPrintf("\"%s\",\"%s\",%i\n",playstring,pl->getCachedNameForEntry(entry), pl->getCachedLengthForEntry(entry));
    FWRITE(info, info.len(), 1, csvFile);
  }

  return 1;
}
