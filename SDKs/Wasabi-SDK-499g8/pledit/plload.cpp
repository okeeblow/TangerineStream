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
#include "plload.h"
#include "../common/playstring.h"

PlaylistLoader::PlaylistLoader() : do_db_add(TRUE) {
}

PlaylistLoader::~PlaylistLoader() {
  entries.deleteAll();
}

void PlaylistLoader::setPlaylistPath(const char *newpath) {
  plpath = newpath;
}

void PlaylistLoader::setPlaylistPathFromFilename(const char *fn) {
  if (!STRNINCMP(fn, "file:")) fn += STRLEN("file:");
  String w = fn;
  char *pt = const_cast<char *>(Std::filename(w));
  if (pt == NULL || pt == w.getValue()) return;
  if (Std::isDirChar(pt[-1])) {
    pt[-1] = 0;
    setPlaylistPath(w);
  }
}

void PlaylistLoader::enableDatabaseAdd(int enabled) {
  do_db_add = enabled;
}

const char *PlaylistLoader::getLabel() {
  return getName();
}

int PlaylistLoader::getNumEntries() {
  return entries.getNumItems();
}

const char *PlaylistLoader::enumEntry(int n) {
  Playstring *str = entries[n];
  if (str == NULL) return NULL;
  return *str;
}

void PlaylistLoader::removeSlashes(char *buf) {
  char *pt = buf;
//  for (; *pt; pt++) if (*pt == '/') *pt = '\\';
  // remove extra slashes, unless they are right after :, like in http://
  for (pt = buf+1; *pt; pt++) {
    if (pt[-1] != ':' && pt[0] == '/' && pt[1] == '/') {
      STRCPY(pt, pt+1);
      pt--;
    }
  }
}

void PlaylistLoader::addPlaylistEntry(const char *playstring, const char *initial_name) {
  if (initial_name != NULL && *initial_name == '\0') initial_name = NULL;
  playstring = adjustPlaystring(playstring, plpath);
  if (do_db_add) api->metadb_addNewItem(playstring, initial_name);
  entries.addItem(new Playstring(playstring));
}

void PlaylistLoader::applyMetaData(const char *playstring, const char *name, const char *data, int datalen, int datatype) {
  playstring = adjustPlaystring(playstring, plpath);
  api->metadb_setMetaData(playstring, name, data, datalen, datatype);
}

void PlaylistLoader::trimWhitespace(char *buf) {
  while (ISSPACE(*buf)) STRCPY(buf, buf+1);
  for (char *t = buf+STRLEN(buf)-1; t >= buf; t--) {
    if (*t == '\n' || *t == '\r' || *t == ' ' || *t == '\t') {
      *t = 0;
    } else break;
  }
}

int PlaylistLoader::needs_file(const char *str) {
  if (STRSTR(str, "://")) return 0;
  if (!STRNNCMP(str, "file:")) return 0;
  if (!STRNNCMP(str, "http:")) return 0;
  if (!STRNNCMP(str, "cdda:")) return 0;
  if (!STRNNCMP(str, "cda:")) return 0;
  return 1;
}

int PlaylistLoader::needs_path(const char *str) {
  if (STRSTR(str, ":/")) return 0;
  if (!STRNINCMP(str, "\\\\")) return 0;	// unc
  if (!STRNNCMP(str, "http:")) return 0;
  if (!STRNINCMP(str, "file:")) return 0;
  if (!STRNINCMP(str, "cdda:")) return 0;
  if (!STRNINCMP(str, "cda:")) return 0;
#ifdef WIN32
  if (ISALPHA(*str) && str[1] == ':') return 0;	// found a drive letter
#endif
  return 1;
}

const char *PlaylistLoader::adjustPlaystring(const char *str, const char *path) {
  if (path == NULL || *path == '\0') return str;
  static String ret;
  int need_path = needs_path(str);
  if (need_path) {
    if (*str != Std::dirChar()) {
      ret.printf("%s%c%s", path, Std::dirChar(), str);
    } else {
#ifdef WIN32
      if (ISALPHA(path[0]) && path[1] == ':')
        ret.printf("%c:%s", path[0], str);
      else
#endif
        ret = str;
    }
  } else {
    ret = str;
  }
  if (needs_file(ret)) ret.prepend("file:");

  if (!STRNINCMP(ret, "http:")) {
    ret.changeChar('\\', '/');// ?
    ret.replace(" ", "%20");
  }

  removeSlashes(ret.getNonConstVal());

  return ret;
}
