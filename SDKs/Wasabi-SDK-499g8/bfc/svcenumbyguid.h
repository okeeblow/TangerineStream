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
#ifndef _SVCENUMBYGUID_H
#define _SVCENUMBYGUID_H

/*<?<autoheader/>*/
/*?>*/

#include "svc_enum.h"

/*
 * this is a helper class to fetch a service * by GUID
 * usage: svc_something *svc = SvcEnumByGuid<svc_something>(svcguid);
 * @short: Helper class to fetch unique service by GUID
*/
template <class SERVICETYPE>
class COMEXP SvcEnumByGuid {
public:
/**
@param _guid The GUID of the service factory to fetch the service from.
*/
  SvcEnumByGuid() : guid(SERVICETYPE::getServiceGuid()) {}
  SvcEnumByGuid(GUID _guid) : guid(_guid) {}

/**
@return The pointer to the service.
*/
  SERVICETYPE *getInterface() {
    waServiceFactory *svc = WASABI_API_SVC->service_getServiceByGuid(guid);
    return castService<SERVICETYPE>(svc);
  }

  operator SERVICETYPE *() { return getInterface(); }

private:
  GUID guid;
};

#endif // _SVCENUMBYGUID_H
