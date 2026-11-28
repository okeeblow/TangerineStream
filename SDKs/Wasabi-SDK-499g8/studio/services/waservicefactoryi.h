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
#ifndef __WASERVICEFACTORY_IMPL_H
#define __WASERVICEFACTORY_IMPL_H

/*<?<autoheader/>*/
#include "waservicefactory.h"
#include "waservicefactoryx.h"

class CfgItem;
/*?>*/
/*[interface.header.h]
#include "common/nsguid.h"
#include "studio/api.h"
*/           
                        
// this is a wasabi service factory. it's a static object you can query to
// fetch the * to your real service interface
class NOVTABLE COMEXP waServiceFactoryI : public waServiceFactoryX {
public:
  virtual ~waServiceFactoryI() {}
protected:
/*[interface.x_getServiceType.h]
  public:
*/
  DISPATCH(100) virtual FOURCC x_getServiceType() { return svc_serviceType(); } // see services.h
  DISPATCH(200) virtual const char *x_getServiceName() { return svc_getServiceName(); }
  DISPATCH(210) virtual GUID getGuid() { return svc_getGuid(); } // GUID per service factory, can be INVALID_GUID
/**
  Fetches a pointer to an instance of the service. Optionally locks it into the
  global table, so that it can be safely released via service_release()
  @see ComponentAPI::service_release()
  @param global_lock If TRUE, calls service_lock() with the service *
  @see releaseInterface()
*/
/*[dispatchable.getInterface.postcall]
  // -- generated code - edit in waservicefactoryi.h
  // support old code that always locks even when global_lock==FALSE
  if (!global_lock && __retval != NULL && !supportNonLockingGetInterface())
    WASABI_API_SVC->service_unlock(__retval);
*/
  DISPATCH(300) virtual void *getInterface(int global_lock = TRUE) { return svc_getInterface(global_lock); }
  DISPATCH(300) virtual void *_RETIRED_getInterface() { return getInterface(); } // for back compat
  DISPATCH(301) virtual int supportNonLockingGetInterface() { return svc_supportNonLockingGetInterface(); }

/**
  Releases a pointer to an instance of the service. Call this when you're done
  with a service interface. Do NOT just call delete on it! Only the original
  service factory can safely delete it. Also, do not pass in any pointers
  that were not allocated from this factory.
  @see getInterface()
  @ret TRUE if no error, FALSE if error.
*/                       
  DISPATCH(310) virtual int releaseInterface(void *ifc) { return svc_releaseInterface(ifc); } // when they're done w/ it

/**
  Fetches a string for optional quick lookup of a service factory. This string
  is defined on a service family type basis. e.g. the xuitag for xuicreators.
*/
  DISPATCH(500) virtual const char *getTestString() { return svc_getTestString(); }
  DISPATCH(600) virtual int serviceNotify(int msg, int param1 = 0, int param2 = 0) { return svc_notify(msg, param1, param2); }
public:

  NODISPATCH virtual FOURCC svc_serviceType()=0; // see services.h
  NODISPATCH virtual const char *svc_getServiceName()=0;
  NODISPATCH virtual GUID svc_getGuid()=0; // GUID per service factory, can be INVALID_GUID
  NODISPATCH virtual void *svc_getInterface(int global_lock = TRUE)=0;
  NODISPATCH virtual int svc_supportNonLockingGetInterface() { return TRUE; }
  NODISPATCH virtual int svc_releaseInterface(void *ifc)=0; // when they're done w/ it
  NODISPATCH virtual CfgItem *svc_getCfgInterface()=0;
  NODISPATCH virtual const char *svc_getTestString()=0;
  NODISPATCH virtual int svc_notify(int msg, int param1 = 0, int param2 = 0)=0;
};

#endif // __WASERVICEFACTORY_IMPL_H
