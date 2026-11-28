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
#ifndef _SVCMGR_H
#define _SVCMGR_H

#include "services/service.h"

class ServiceManager {
public:
  static int registerService(waServiceFactory *service, GUID owner);
  static int deregisterService(waServiceFactory *service, int internal = 0);

  static int getNumServices(FOURCC svc_type);
  static waServiceFactory *enumService(FOURCC svc_type, int n);

  static waServiceFactory *getServiceByGuid(GUID guid);

  static void sendNotification(int msg, int param1 = 0, int param2 = 0);

  static int lock(waServiceFactory *owner, void *svcptr);
  static int unlock(void *svcptr);
  static int clientLock(void *svcptr, GUID lockedby);
  static int release(void *svcptr);

  static GUID getOwningComponent(void *svcptr);
  static GUID getLockingComponent(void *svcptr);
  static const char *getServiceTypeName(FOURCC svc_type);

  static void onShutdown();

  static FOURCC safe_getServiceType(waServiceFactory *was);
  static const char *safe_getServiceName(waServiceFactory *was);

  static int getNumServices();
  static int getNumServicesByGuid();
  static int getNumOwners();
  static int getNumLocks();

  static int isValidService(FOURCC svctype, waServiceFactory *service);
};

#endif
