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
#ifndef _QUERY_H
#define _QUERY_H

/* Do NOT put this in common.dll! It needs to use the component's copy of
the API pointer! */

#include "ptrlist.h"
#include "string.h"
#include "../common/playstring.h"

class Query {
public:
  Query();
  virtual ~Query();

  int setTable(GUID table_guid);
  int setQueryText(const char *querytext);
  const char *getQueryText() const { return query_text; }

  void setIndexName(const char *name) { idxname = name; }
  void setIndexValue(const char *val) { idxval = val; }

  void refresh();
  void drop();

  int getNumItems() const { return list.getNumItems(); }
  const char *enumItem(int n) { return list[n]->getValue(); }

private:
  GUID table_guid;
  String query_text;
  String idxname, idxval;
  PtrListQuickSorted<Playstring, PlaystringComparator> list;
};

#endif
