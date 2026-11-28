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
#include "svccache.h"
#include "string.h"

#if !defined(WASABI_API_SVC)
#error no no
#endif

SvcCache::SvcCache(FOURCC type) {
  int p = 0;
  list.setAutoSort(FALSE);
  for (;;) {
    waServiceFactory *svc = WASABI_API_SVC->service_enumService(type, p++);
    if (svc == NULL) break;
    const char *teststr = svc->getTestString();
    if (teststr == NULL || *teststr == '\0') continue;
//CUT DebugString("cachin %s\n", teststr);
    list.addItem(svc);
  }
  list.setAutoSort(TRUE);
}

waServiceFactory *SvcCache::findServiceFactory(const char *searchval) {
  return list.findItem(searchval);
}

int SvcCache::waServiceFactoryCompare::compareItem(waServiceFactory *p1, waServiceFactory* p2) {
  int r = STRICMP(p1->getTestString(), p2->getTestString());
  if (r == 0) return CMP3(p1, p2);
  return r;
}

int SvcCache::waServiceFactoryCompare::compareAttrib(const char *attrib, waServiceFactory *item) {
  return STRICMP(attrib, item->getTestString());
}
