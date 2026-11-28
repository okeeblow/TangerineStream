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

#include "stringtable.h"

StringRef::StringRef(const char *str)
  : String(str) {
  refcount = 1;
}

StringRef::~StringRef() {
  //ASSERT(refcount == 0);
  if (refcount != 0) DebugString("refcount != 0\n");
}

StringTable::~StringTable() {
//  ASSERTPR(refs.getNumItems() == 0, "didn't free all strings!");
  refs_freelist.deletePtrList(&refs);
}

StringTable::StringTable(int autocull) : want_autocull(autocull) {}

const char *StringTable::allocStr(const char *val) {
  if (val == NULL) return NULL;
  StringRef *ref=refs.findItem(val);
  if(ref) {
    ref->refcount++;
    return ref->getValue();
  }

  StringRef *s = refs_freelist.newRecord(val);
  refs.addItem(s);
  return s->getValue();
}

void StringTable::deallocStr(const char *val) {
  if (val == NULL) return;

  int i=0;
  StringRef *ref=refs.findItem(val,&i);
  if(ref) {
    if (--ref->refcount <= 0) {
      if (want_autocull) {
        refs.delByPos(i);
        refs_freelist.deleteRecord(ref);
      }
    }
    return;
  }
}

void StringTable::cull() {
  for (int i = 0; i < refs.getNumItems(); i++) {
    StringRef *ref = refs[i];
    if (ref->refcount == 0) {
      refs.delByPos(i);
      i--;
      refs_freelist.deleteRecord(ref);
    }
  }
}

int StringTable::gccb_onGarbageCollect() {
  cull();
  return 1;
}
