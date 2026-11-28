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
#ifndef _OPENDIR_H
#define _OPENDIR_H

#include "../studio/services/svc_filesel.h"
#include "../bfc/string.h"
#include "../bfc/ptrlist.h"

class OpenDirectory : public svc_fileSelectorI {
public:
  static const char *getServiceName() { return "Open Directory Selector"; }

  OpenDirectory();
  virtual ~OpenDirectory();

  virtual int testPrefix(const char *prefix) { return STRCASEEQL(prefix, "directory"); }
  virtual const char *getPrefix() { return "directory"; }
  virtual int setExtList(const char *ext) { return 1; }
  virtual int runSelector(RootWnd *parWnd, int type, int allow_multiple, const char *ident=NULL, const char *default_dir=NULL);
  virtual int getNumFilesSelected() { return files.getNumItems(); }
  virtual const char *enumFilename(int n) { if(n>=files.getNumItems()) return NULL; return files[n]->getValue(); }
  virtual const char *getDirectory() { return directory; }

private:
  void addDir(const char *path);
  void addFile(const char *filename);
  void cleanFiles();

  // checked during addDir()
  int recurseSubDirs;
  PtrList<char> exts;
  
  PtrListQuickSorted<String, StringComparator> files;
  String directory;
};

#endif
