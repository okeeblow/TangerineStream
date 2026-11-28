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
#ifndef _OPENFILE_H
#define _OPENFILE_H

#include "../common/modal.h"
#include "../bfc/ptrlist.h"
#include "../bfc/string.h"
#include "../bfc/memblock.h"
#include "../studio/services/svc_filesel.h"

class RootWnd;

class OpenFileWnd : public Modal {
public:
  OpenFileWnd(const char *ident=NULL, const char *default_dir=NULL);
  ~OpenFileWnd();

  int getOpenFile(RootWnd *parWnd, const char *ext, int chosenfilter=0, int multisel=0);
  int getNumFilenames() const { return filenames.getNumItems(); }
  const char *enumFilename(int i);
  const char *getDirectory() { return directory; }

private:
  // for internationalization
  int getOpenFileW(RootWnd *parWnd, const char *ext, int chosenfilter=0, int multisel=0);

  String identifier;
  String defaultdir;
  PtrListQuickSorted<String, StringComparator> filenames;
  String directory;
};

class DefOpen : public svc_fileSelectorI {
public:
  static const char *getServiceName() { return "Default File Selector"; }

  DefOpen();
  virtual ~DefOpen();

  virtual int testPrefix(const char *prefix);
  virtual const char *getPrefix();
  virtual int setExtList(const char *ext);
  virtual int runSelector(RootWnd *parWnd, int type, int allow_multiple, const char *ident=NULL, const char *default_dir=NULL);
  virtual int getNumFilesSelected();
  virtual const char *enumFilename(int n);
  virtual const char *getDirectory() { return directory; }

private:
  int _type;
  OpenFileWnd *op;
  MemBlock<char> extlist;
  String directory;
};

#endif
