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

#include "plhtml.h"
#include "playlist.h"
#include <bfc/attribs/attribs.h>

#define BUFSIZE 32767
#define ANCHOR "<a href=\""
#define HTTP "http://"

extern _bool http_recurse;

int HtmlLoader::readPlaylist(const char *filename) {
  FILE *fp = FOPEN(filename, "rt");
  if (fp == NULL) return -1;

  String path(filename);
  char *pt = const_cast<char *>(Std::filename(path));
  if (pt == NULL || path.isequal(pt)) return 0;
  *pt = 0;	// cheesy :(

  int count = 0;
  MemBlock<char> buf(BUFSIZE);
  while (FGETS(buf, buf.getSizeInBytes()-1, fp)) {
    const char *str = buf;
    for (;;) {
      str = STRCASESTR(str, ANCHOR);
      if (str == NULL) break;
      str += STRLEN(ANCHOR);
      String copy = str;
      copy.truncateOnChar('\"');
      str += copy.len();

      if (STREQL(copy, "..") || !STRNNCMP(copy, "../") || STRSTR(copy, "/..")) continue;

      String initial_name;
      const char *pn = STRSTR(str, ">");
      if (pn != NULL) {
        initial_name = pn+1;
        initial_name.truncateOnChar('<');
        initial_name.replace("%20", " ");
        initial_name.replace("&apos;", "'");
        initial_name.replace("&quot;", "\"");
        initial_name.replace("&gt;", ">");
        initial_name.replace("&lt;", "<");
        initial_name.replace("&amp;", "&");//LAST!
        if (initial_name.isempty()) initial_name = NULL;
      }

      if (copy.lastChar() == '/' && http_recurse) {
        copy.cat("?index.html");
      }

      const char *ext = Std::extension(copy);
      if (ext == NULL) continue;
      if (api->core_getExtensionFamily(ext) == NULL) continue;

      if (STRNINCMP(copy, HTTP)) {	// relative path
//CUT        if (Std::isDirChar(*copy.getValue())) {// relative to server
//CUT        }
        copy.prepend(path);
      } else continue;	// only relative!
      api->metadb_addNewItem(copy, initial_name);
      addPlaylistEntry(copy);
      count++;
    }
  } 

  FCLOSE(fp);
  return count;
}

#include "../bfc/util/timefmt.h"
#include "../bfc/xml/xmlwrite.h"

int HtmlWriter::writePlaylist(const char *filename, Playlist *pl, int full_data, int first, int last) {
  FILE *fp = FOPEN(filename, "wt");
  if (!fp) return 0;

	FPRINTF(fp,
		"<html><head><link rel=\"stylesheet\" href=\"null\"><style TYPE=\"text/css\"><!--BODY { background: #000040; }\n"
		".para1 { margin-top: -42px; margin-left: 145px; margin-right: 10px; font-family: \"font2, Arial\"; font-size: 30px; line-height: 35px; text-align: left; color: #E1E1E1; }\n"
		".para2 { margin-top: 15px; margin-left: 15px; margin-right: 50px; font-family: \"font1, Arial Black\"; font-size: 50px; line-height: 40px; text-align: left; color: #004080; }\n"
		"--></style><title>Winamp3 Generated Playlist - "
);
  XMLWrite::efprintf(fp, "%s", pl->getPlaylistName());
FPRINTF(fp, "</title></head>"
		"<body BGCOLOR=\"#000080\" topmargin=\"0\" leftmargin=\"0\" text=\"#FFFFFF\"><!--TOOLBAR_START--><!--TOOLBAR_EXEMPT-->\n"
		"<div align=\"center\"><div CLASS=\"para2\" align=\"center\"><p>WINAMP3</p></div><div CLASS=\"para1\" align=\"center\">\n"
		"<p>playlist</p></div></div><hr align=\"left\" width=\"90%%\" noshade size=\"1\" color=\"#FFBF00\"><div align=\"right\">\n"
		"<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"98%%\"><tr><td><small><small><font face=\"Arial\" color=\"#FFBF00\">\n"
		);
  FPRINTF(fp, "Playlist name: <font color=\"#ffffff\">");
  XMLWrite::efprintf(fp, "%s", pl->getPlaylistName());
  FPRINTF(fp, "</font><br>Number of entries: <font color=\"#ffffff\">%d</font><br>\n", pl->getNumEntries());

  FPRINTF(fp, 
"<big>Playlist files:</big></font><ul><font face=\"Arial\" color=\"#FFFFFF\"><small>"/*,filename*/);
  for (int i = first; i <= last; i++) {
    PlaylistEntry *entry = pl->enumEntry(i);
    const char *name = pl->getCachedNameForEntry(entry);
    XMLWrite::efprintf(fp, "%s", name);	// do escaped printing
    int l = pl->getCachedLengthForEntry(entry);
    if (l > 0) {
      char buf[MAX_PATH];
      TimeFmt::printMinSec(l/1000, buf, MAX_PATH);
      FPRINTF(fp, " <font size=\"-1\">(%s)</font>", buf);
    }
    FPRINTF(fp, "<BR>\n");
  }

  FPRINTF(fp,"</font></ul></blockquote><hr align=\"left\" width=\"90%%\" noshade size=\"1\" color=\"#FFBF00\"></body></html>");
  FCLOSE(fp);

  return 1;
}
