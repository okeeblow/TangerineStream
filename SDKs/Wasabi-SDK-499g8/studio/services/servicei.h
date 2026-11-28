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
#ifndef _SERVICEI_H
#define _SERVICEI_H

// here is where we define some helper implementations of the service factory
// interface

#include "service.h"
#include "../api.h"
#include "waservicefactoryi.h"
#include "waservicefactorybase.h"
#include "waservicefactoryt.h"
#include "waservicefactorytsingle.h"
#include "bfc/ptrlist.h"
#include "bfc/foreach.h"

// DEPRECATED
#define waService waServiceFactory
#define waServiceT waServiceFactoryT
#define waServiceTSingle waServiceFactoryTSingle

// Declaring services via macros
// ie:
// BEGIN_SERVICES(YourClass_Svc)
// DECLARE_SERVICE(SomeSvcCreator<YourServiceClass1>);
// DECLARE_SERVICE(SomeSvcCreator<YourServiceClass2>);
// DECLARE_SERVICE(SomeSvcCreator<YourServiceClass3>);
// DECLARE_SERVICETMULTI(svc_serviceclass, YourMultiInstanceServiceClass1>);
// DECLARE_SERVICETMULTI(svc_serviceclass, YourMultiInstanceServiceClass2>);
// DECLARE_SERVICETMULTI(svc_serviceclass, YourMultiInstanceServiceClass3>);
// DECLARE_SERVICETSINGLE(svc_serviceclass, YourSingleInstanceServiceClass1>);
// DECLARE_SERVICETSINGLE(svc_serviceclass, YourSingleInstanceServiceClass2>);
// DECLARE_SERVICETSINGLE(svc_serviceclass, YourSingleInstanceServiceClass3>);
// END_SERVICES(YourClass_Svc, _YourClass_Svc);

// The macro DECLARE_MODULE_SVCMGR should be defined in your main cpp file if you are compiling
// a standalone exe without using the Application class. In this case you should also add
// MODULE_SVCMGR_ONINIT(); in your app startup and MODULE_SVCMGR_ONSHUTDOWN() in your app shutdown
// (class Application does this already)

// You can use BEGIN_SYS_SERVICES to declare a service that is initialized in priority and 
// shutdown late (normal version inits late, shutdowns early)


class StaticServices {
  public:
    virtual void registerServices()=0;
    virtual void unregisterServices()=0;
};

extern PtrList<StaticServices> __g_module_services;
extern PtrList<StaticServices> __g_module_services2;

#define DECLARE_MODULE_SVCMGR \
PtrList<StaticServices> __g_module_services; \
PtrList<StaticServices> __g_module_services2; 

#define MODULE_SVCMGR_ONINIT() { \
foreach(__g_module_services) \
  __g_module_services.getfor()->registerServices(); \
endfor; \
}

#define MODULE_SVCMGR_ONINIT2() { \
foreach(__g_module_services2) \
  __g_module_services2.getfor()->registerServices(); \
endfor; \
}

#define MODULE_SVCMGR_ONSHUTDOWN() { \
foreach(__g_module_services) \
  __g_module_services.getfor()->unregisterServices(); \
endfor; \
}
 
#define MODULE_SVCMGR_ONSHUTDOWN2() { \
foreach(__g_module_services2) \
  __g_module_services2.getfor()->unregisterServices(); \
endfor; \
}
 

#define BEGIN_SERVICES(CLASSNAME) \
class CLASSNAME : public StaticServices { \
public: \
  CLASSNAME() { \
    __g_module_services.addItem(this); \
  } \
  virtual ~CLASSNAME() {} \
  virtual void registerServices() {

#define BEGIN_SYS_SERVICES(CLASSNAME) \
class CLASSNAME : public StaticServices { \
public: \
  CLASSNAME() { \
    __g_module_services2.addItem(this); \
  } \
  virtual ~CLASSNAME() {} \
  virtual void registerServices() {

#define DECLARE_SERVICE(INSTANTIATIONCODE) \
    registerService(new INSTANTIATIONCODE); 
  
#define DECLARE_SERVICETMULTI(SVCTYPE, SVCCLASS) \
    registerService(new waServiceFactoryT<SVCTYPE, SVCCLASS>); 

#define DECLARE_SERVICETSINGLE(SVCTYPE, SVCCLASS) \
    registerService(new waServiceFactoryTSingle<SVCTYPE, SVCCLASS>); 
  
#define END_SERVICES(CLASSNAME, INSTANCENAME) \
  } \
  virtual void unregisterServices() { \
    foreach(services) \
      waServiceFactoryI *svc = services.getfor(); \
      WASABI_API_SVC->service_deregister(svc); \
      delete svc; \
    endfor; \
    services.removeAll();\
  } \
private: \
  void registerService(waServiceFactoryI *svc) { \
    if (svc != NULL) { \
      services.addItem(svc); \
      WASABI_API_SVC->service_register(svc); \
    } \
  } \
  PtrList<waServiceFactoryI> services; \
}; \
CLASSNAME INSTANCENAME;

#endif
