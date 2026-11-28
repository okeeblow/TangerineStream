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
#ifndef _STRINGTABLE_H
#define _STRINGTABLE_H

// maintains a refcounted cache of strings

#include "../bfc/ptrlist.h"
#include "../bfc/string.h"
#include "../bfc/freelist.h"
#include "../studio/gccb.h"	// for auto-garbage collection

//class StringRef;
class StringRef : public String {
protected:
  friend class StringTable;
  friend class Freelist<StringRef>;

  StringRef(const char *str);
  ~StringRef();

  int refcount;
};

class PLS_StringRef  {
public:
  // comparator for sorting
  static int compareItem(StringRef *p1, StringRef *p2) {
    return STRCMP(p1->getValue(), p2->getValue());
  }
  // comparator for searching
  static int compareAttrib(const char *attrib, StringRef *item) {
    return STRCMP(attrib, item->getValue());
  }
};

class COMEXP StringTable : public GarbageCollectCallbackI {
public:
  StringTable(int autocull=TRUE);
  ~StringTable();

  const char *allocStr(const char *val);
  void deallocStr(const char *val);

  // delete strings with refcount == 0 (needed if autocull==FALSE)
  void cull();

protected:
  // if you want, you can register this as a syscallback and it will cull
  // on system garbage collection events (instead of on deletion)
  virtual int gccb_onGarbageCollect();

private:
  int want_autocull;
  PtrListInsertSorted<StringRef,PLS_StringRef> refs;
  Freelist<StringRef> refs_freelist;
};

#endif
