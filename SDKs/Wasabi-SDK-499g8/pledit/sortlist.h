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
#ifndef _SORTLIST_H
#define _SORTLIST_H

#include "../bfc/ptrlist.h"
#include "../common/playitem.h"
#include "../studio/api.h"

// various sorted pointer lists

class PtrListSorted_PlayString : public PtrListQuickSorted<PlayItem> {
protected:
  // comparator for sorting
  virtual int compareItem(PlayItem *p1, PlayItem* p2) {
    return STRCMP(p1->getPlayString(), p2->getPlayString());
  }
  // comparator for searching
  virtual int compareAttrib(char *attrib, PlayItem *item) {
    return STRCMP(attrib, item->getPlayString());
  }
};

class PtrListSorted_MetaData : public PtrListQuickSorted<PlayItem> {
public:
  PtrListSorted_MetaData() {
    tag = NULL;
    tagtype = MDT_ERROR;
  }
  virtual ~PtrListSorted_MetaData() {
    FREE(tag);
  }

  void setSortTag(char *newtag) {
    ASSERT(newtag != NULL);
    FREE(tag);
    tag = STRDUP(newtag);
    tagtype = api->metadb_getMetaDataType(tag);
  }

protected:
  // comparator for sorting
  virtual int compareItem(PlayItem *p1, PlayItem* p2) {
    char buf1[WA_MAX_PATH]="";
    char buf2[WA_MAX_PATH]="";
    int r1, r2;
    r1 = api->metadb_getMetaData(p1, tag, buf1, WA_MAX_PATH);
    r2 = api->metadb_getMetaData(p2, tag, buf2, WA_MAX_PATH);
    switch (tagtype) {
      case MDT_STRINGZ:
        return STRCMP(buf1, buf2);
      case MDT_INT:
      case MDT_TIME:
      case MDT_TIMESTAMP: {
        int a = *(int *)buf1, b = *(int *)buf2;
        if (a < b) return -1;
        else if (a > b) return 1;
        return 0;
      }
      default:
        return 0;
    }
  }
  // comparator for searching
  virtual int compareAttrib(char *attrib, PlayItem *item) {
ASSERTPR(0, "don't");
    return STRCMP(attrib, item->getPlayString());
  }

private:
  char *tag;
  int tagtype;
};

#endif
