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
#ifndef __WASERVICEFACTORYBASE_IMPL_H
#define __WASERVICEFACTORYBASE_IMPL_H

/*<?<autoheader/>*/
class CfgItem;
/*?>*/

#include "waservicefactoryi.h"

template <class SERVICETYPE, class SERVICE>
class NOVTABLE waServiceFactoryBaseX : public waServiceFactoryI {
public:
  waServiceFactoryBaseX(GUID myGuid = INVALID_GUID) : guid(myGuid) {}
  virtual FOURCC svc_serviceType()=0;
  virtual const char *svc_getServiceName() { return SERVICE::getServiceName(); }
  virtual GUID svc_getGuid() { return guid; }
  virtual void *svc_getInterfaceAndLock() {// this is only for back compat
    return getInterface(TRUE);
  }

  virtual void *svc_getInterface(int global_lock)=0;
  virtual int svc_releaseInterface(void *ptr)=0;

  virtual CfgItem *svc_getCfgInterface() { return NULL; }
  virtual const char *svc_getTestString() { return NULL; }

  virtual int svc_notify(int msg, int param1 = 0, int param2 = 0) { return 0; }

private:
  GUID guid;
};

// if you derive from this, all you have to do is override the newService()
// and delService() methods... if all you need to do is instantiate a class
// and destroy it use the helper below (waServiceFactoryT)
template <class SERVICETYPE, class SERVICE>
class NOVTABLE waServiceFactoryBase : public waServiceFactoryBaseX<SERVICETYPE, SERVICE> {
public:
  waServiceFactoryBase(GUID myGuid = INVALID_GUID) : waServiceFactoryBaseX<SERVICETYPE, SERVICE>(myGuid) {}
  virtual FOURCC svc_serviceType() { return SERVICETYPE::getServiceType(); }
  virtual void *svc_getInterface(int global_lock) { // new style, client optionally does the locking
    SERVICETYPE *ret = newService();
    if (global_lock) WASABI_API_SVC->service_lock(this, ret);
    return ret;
  }
  virtual int svc_releaseInterface(void *ptr) {
    return delService(static_cast<SERVICE*>(static_cast<SERVICETYPE*>(ptr)));
  }
protected:
  virtual SERVICETYPE *newService()=0;
  virtual int delService(SERVICETYPE *service)=0;
};

// and this one just exposes a service pointer, without factory. note that
// SERVICETYPE and SERVICE do not have to be related (unlike waServiceFactoryBase
// who needs a relationship between the classes in releaseInterface)
template <class SERVICETYPE, class SERVICE>
class NOVTABLE waServiceBase : public waServiceFactoryBaseX<SERVICETYPE, SERVICE> {
public:
  waServiceBase(GUID myGuid = INVALID_GUID) : waServiceFactoryBaseX<SERVICETYPE, SERVICE>(myGuid) {}
  virtual FOURCC svc_serviceType() { return SERVICE::getServiceType(); }
  virtual void *svc_getInterface(int global_lock) { // new style, client optionally does the locking
    SERVICETYPE *ret = getService();
    if (global_lock) WASABI_API_SVC->service_lock(this, ret);
    return ret;
  }
  virtual int svc_releaseInterface(void *ptr) {
    return TRUE;
  }
protected:
  virtual SERVICETYPE *getService()=0;
};



#endif // __WASERVICEFACTORYBASE_IMPL_H
