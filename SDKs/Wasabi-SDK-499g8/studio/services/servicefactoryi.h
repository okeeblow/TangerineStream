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
#include "servicefactory.h"
#include "servicefactoryx.h"

class CfgItem;
/*?>*/
/*[interface.header.h]
#include "common/nsguid.h"
*/
                        
class waServiceFactoryI : public waServiceFactoryX {
protected:
  DISPATCH(100) virtual FOURCC svc_serviceType()=0;
  DISPATCH(200) virtual const char *svc_getServiceName()=0;
  DISPATCH(210) virtual GUID svc_getGuid()=0;
  DISPATCH(300) virtual void *_RETIRED_svc_getInterface()=0;// for back compat
  DISPATCH(300) virtual void *svc_getInterface(int global_lock = TRUE)=0;
  DISPATCH(301) virtual int svc_supportNonLockingGetInterface() { return TRUE; }
  DISPATCH(310) virtual int svc_releaseInterface(void *ifc)=0;
  DISPATCH(400) virtual CfgItem *svc_getCfgInterface()=0;
  DISPATCH(500) virtual const char *svc_getTestString()=0;
  DISPATCH(600) virtual int svc_notify(int msg, int param1 = 0, int param2 = 0)=0;
};

#endif // __WASERVICEFACTORY_IMPL_H
