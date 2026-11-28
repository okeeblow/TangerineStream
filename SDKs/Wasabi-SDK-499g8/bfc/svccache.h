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
#ifndef _SVC_CACHE_H
#define _SVC_CACHE_H

#include "svc_enum.h"
#include "ptrlist.h"

/**
  This is a caching version of SvcEnum. Upon creation, it enumerates all
  service factories in the family and keeps them in a list. Then you can
  call findService() with a search string to quickly find the service you
  want. If you don't have a search string, you can still use a SvcEnum.
*/

class COMEXP SvcCache {
protected:
  SvcCache(FOURCC type);
 
public:
  waServiceFactory *findServiceFactory(const char *searchval);

private:
  class waServiceFactoryCompare {
  public:
    static int compareItem(waServiceFactory *p1, waServiceFactory* p2);
    static int compareAttrib(const char *attrib, waServiceFactory *item);
  };
  PtrListQuickSorted<waServiceFactory, waServiceFactoryCompare> list;
};

template <class T>
class SvcCacheT : public SvcCache {
public:
  SvcCacheT() : SvcCache(T::getServiceType()) { }

  T *findService(const char *key, int global_lock=TRUE) {
    waServiceFactory *sf = findServiceFactory(key);
    if (sf == NULL) return NULL;
    T *ret = castService<T>(sf, global_lock);
    return ret;
  }
};

#endif
