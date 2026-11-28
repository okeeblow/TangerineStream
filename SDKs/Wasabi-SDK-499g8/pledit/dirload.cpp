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

#include "dirload.h"
#include "../common/readdir.h"
#include "../common/playstring.h"
#include <bfc/attribs/attrbool.h>
#include "../bfc/util/pathparse.h"

int DirLoader::testFilename(const char *filename) {
  if (!STRNINCMP(filename, "file:")) filename += STRLEN("file:");

  return Std::isDirectory(filename);
}

int DirLoader::readPlaylist(const char *ofilename) {
  if (!STRNINCMP(ofilename, "file:")) ofilename += STRLEN("file:");
  String filename(ofilename);
  if (filename.lastChar() == DIRCHAR) filename.trunc(-1);
  ReadDir dir(ofilename);
  int n = 0;

  PtrListQuickSorted<Playstring, PlaystringComparator> flist;

  extern _bool incpl;
  int dpl = incpl;

  while (dir.next()) {
    const char *fn = dir.getFilename();
    ASSERT(fn != NULL);

    int dontadd = 0;
    if (!dpl) {
      const char *fam = api->core_getExtensionFamily(Std::extension(fn));
      if (fam != NULL && *fam != '\0') {
        PathParser pp(fam);
        for (int i = 0, n = pp.getNumStrings(); i < n; i++) {
          if (STRCASEEQL(pp.enumString(i), "Playlist")) dontadd = 1;
        }
      }
    }

    if (dontadd) continue;

    StringPrintf ps("file:%s%s%s", filename, DIRCHARSTR, fn);
    flist.addItem(new Playstring(ps));
    n ++;
  }

  // flist implicit sort here

  foreach(flist)
    addPlaylistEntry(*flist.getfor());
  endfor

  flist.deleteAll();

  return n;
}
