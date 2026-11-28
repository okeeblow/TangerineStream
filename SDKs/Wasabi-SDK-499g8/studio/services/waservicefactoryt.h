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
#ifndef __WASERVICEFACTORYT_IMPL_H
#define __WASERVICEFACTORYT_IMPL_H

/*<?<autoheader/>*/
/*?>*/

#include "waservicefactorybase.h"

// this is a service factory template that will manufacture any number
// of a given class SERVICE, which is derived from service class SERVICETYPE
template <class SERVICETYPE, class SERVICE>
class waServiceFactoryT : public waServiceFactoryBase<SERVICETYPE, SERVICE> {
public:
  waServiceFactoryT(GUID myGuid = INVALID_GUID) :
    waServiceFactoryBase<SERVICETYPE, SERVICE>(myGuid) {}
  virtual SERVICETYPE *newService() {
    SERVICE *ret = new SERVICE;
    ASSERT(ret != NULL);
    return ret;
  }
  virtual int delService(SERVICETYPE *service) {
    ASSERT(service != NULL);
    delete static_cast<SERVICE*>(service);
    return 1;
  }
};

#endif // __WASERVICEFACTORYT_IMPL_H
