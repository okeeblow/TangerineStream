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

#include "queryload.h"

#include "../bfc/db/scanner.h"
#include "../bfc/paramparser.h"
#include "../common/nsguid.h"

#define PREFIX "query://"

int QueryLoader::testFilename(const char *filename) {
  return !STRNINCMP(filename, PREFIX);
}

int QueryLoader::readPlaylist(const char *filename) {
  const GUID baseGUID = 
  { 0x3cbd4483, 0xdc44, 0x11d3, { 0xb6, 0x8, 0x0, 0x0, 0x86, 0x34, 0x8, 0x85 } };
  String query = filename;
  // find the prefix
  const char *begin = STRSTR(filename, PREFIX);
  if (begin == NULL) return -1;
  if (begin != NULL) query = begin;
  // strip it
  if (!STRNINCMP(query, PREFIX))
    query = String(filename+STRLEN(PREFIX));
  // strip extension too
  if (!STRNINCMP(Std::extension(filename), "nsq"))
    query.trunc(-4);	//".nsq"
  dbScanner scan(baseGUID);
  
  String multiquery;
  ParamParser pp(query);
  for (int i=0;i<pp.getNumItems();i++) {
    if (nsGUID::fromChar(pp.enumItem(i)) != INVALID_GUID) continue;
    if (!multiquery.isempty()) multiquery += " AND ( ";
    else multiquery += "( ";
    multiquery += pp.enumItem(i);
    multiquery += " )";
  }

  scan.setQuery(multiquery);
  OutputDebugString(multiquery); OutputDebugString("\n");
  int count = 0;
  for (scan.first(); !scan.eof(); scan.next()) {
    addPlaylistEntry(scan.getPlaystring());
    count++;
  }
  return count;
}
