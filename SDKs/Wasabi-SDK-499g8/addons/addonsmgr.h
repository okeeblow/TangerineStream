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
#ifndef __BOOKMARK_H
#define __BOOKMARK_H

#include <bfc/ptrlist.h>
#include <bfc/string.h>
#include <studio/skincb.h>
#include <bfc/attribs/cfgitemi.h>

#include "addonreader.h"

class AddOnsList;

#define TYPE_GLOBAL   0
#define TYPE_THISSKIN 1

// -----------------------------------------------------------------------
class AddOnsMgr: public CfgItemI, public SkinCallbackI {
public:
  AddOnsMgr();
  ~AddOnsMgr();

  void loadAll();
  void saveAll();

  AddOnInfo *enumEntry(int type, int n);
  AddOnInfo *getFromSkinPart(int partid);

  void insertMod(int type, const char *file, int save=1);
  void removeMod(int partid);
  void refreshAllLists();
  void aboutEntry(int partid);
  void refreshList(AddOnsList *l, int seemless=1);

  int skincb_onGuiLoaded();

  void registerList(AddOnsList *l);
  void unregisterList(AddOnsList *l);
  void add(int type, int x, int y);

private:

  PtrList<AddOnInfo> addons[2];
  PtrList<AddOnsList> lists;
};

extern AddOnsMgr addonsmgr;
#endif