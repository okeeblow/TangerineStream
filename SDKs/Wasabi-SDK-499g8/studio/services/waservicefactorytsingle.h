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
#ifndef __WASERVICEFACTORYTSINGLE_IMPL_H
#define __WASERVICEFACTORYTSINGLE_IMPL_H

/*<?<autoheader/>*/
/*?>*/

#include "waservicefactorybase.h"

#include <studio/assert.h>

// this is a service factory template that holds one copy of a class
// and reissues the pointer as needed, with reference counting
template <class SERVICETYPE, class SERVICE>
class waServiceFactoryTSingle : public waServiceFactoryBase<SERVICETYPE, SERVICE> {
public:
  waServiceFactoryTSingle(GUID myGuid = INVALID_GUID) :
    refcount(0), waServiceFactoryBase<SERVICETYPE, SERVICE>(myGuid) {
    singleService = new SERVICE;
  }
  waServiceFactoryTSingle(SERVICE *svc, GUID myGuid = INVALID_GUID) :
    refcount(0), singleService(svc),
    waServiceFactoryBase<SERVICETYPE, SERVICE>(myGuid) { }
  ~waServiceFactoryTSingle() {
    delete singleService;
  }
  virtual SERVICETYPE *newService() {
    ASSERT(singleService != NULL);
    refcount++;
    return singleService;
  }
  virtual int delService(SERVICETYPE *service) {
    ASSERT(static_cast<SERVICE*>(service) == singleService);
    refcount--;
    ASSERT(refcount >= 0);
    return 1;
  }

  SERVICE *getSingleService() { return singleService; }

private:
  SERVICE * singleService;
  int refcount;
};



#endif // __WASERVICEFACTORYTSINGLE_IMPL_H
