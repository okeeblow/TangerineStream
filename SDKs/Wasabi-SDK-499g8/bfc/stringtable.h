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
#ifndef __STRINGTABLE_H
#define __STRINGTABLE_H

#include "string.h"
#include "ptrlist.h"

// a convenient class to statically declare strings and their IDs with binary search lookups
//
// example of use :
//
// BEGIN_STRINGTABLE(_identifier)
// STI("somestring",              0);
// STI("someotherstring",         1);
// STI("andsomeotherstringagain", 2);
// END_STRINGTABLE(_identifier, identifier)

// foo (const char *str) {
//  int a = identifier.getId(str);
//  if (a < 0) { 
//    // not found!
//  }
// return a;
// }

class StringTableItem {
  public:
    StringTableItem(const char *string, int stringid) : str(string), id(stringid) {}
    virtual ~StringTableItem() {}
    const char *getString() { return str; }
    int getId() { return id; }
  private:
    String str;
    int id;
};

class StringTableItemCompare {
public:
  static int compareItem(void *p1, void *p2) {
    return STRICMP(((StringTableItem *)p1)->getString(), ((StringTableItem *)p2)->getString());
  }
  static int compareAttrib(const char *attrib, void *item) {
    return STRICMP(attrib, ((StringTableItem *)item)->getString());
  }
};

class StringTable {
  public:
    StringTable() {}
    virtual ~StringTable() { items.deleteAll(); }
    virtual void addItem(const char *str, int id) { ASSERT(id != -1); items.addItem(new StringTableItem(str, id)); }
    virtual int getId(const char *str) { StringTableItem *i = items.findItem(str); if (i == NULL) return -1; return i->getId(); }
  private:
    PtrListQuickSorted<StringTableItem, StringTableItemCompare> items;
};

#define BEGIN_STRINGTABLE(class_ident) \
class class_ident : public StringTable { \
  public: \
    class_ident() { 

#define STI(str, id) \
      addItem(str, id);

#define END_STRINGTABLE(class_ident, instance_ident) \
    }; \
    virtual ~class_ident() { } \
}; \
\
class_ident instance_ident;


#endif
