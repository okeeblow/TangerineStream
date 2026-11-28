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
#ifndef _SVCCB_H
#define _SVCCB_H

#include "syscbi.h"

namespace SvcCallback {
  enum {
    ONREGISTER=10,
    ONDEREGISTER=20,
  };
};

#define SVCCALLBACK_PARENT SysCallbackI
class SvcCallbackI : public SVCCALLBACK_PARENT {
protected:
  SvcCallbackI() { }

public:
  virtual void svccb_onSvcRegister(FOURCC type, waServiceFactory *svc) { }
  virtual void svccb_onSvcDeregister(FOURCC type, waServiceFactory *svc) { }

private:
  virtual FOURCC syscb_getEventType() { return SysCallback::SERVICE; }

  virtual int syscb_notify(int msg, int param1, int param2) {
    switch (msg) {
      case SvcCallback::ONREGISTER:
        svccb_onSvcRegister((FOURCC)param1, reinterpret_cast<waServiceFactory*>(param1));
      break;
      case SvcCallback::ONDEREGISTER:
        svccb_onSvcRegister((FOURCC)param1, reinterpret_cast<waServiceFactory*>(param1));
      break;
      default: return 0;
    }
    return 1;
  }
};

#endif
