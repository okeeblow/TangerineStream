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
#ifndef _SVC_ACCESSIBILITY_H
#define _SVC_ACCESSIBILITY_H

#include "../../bfc/dispatch.h"
#include "services.h"

class Accessible;
class RootWnd;

class NOVTABLE svc_accessibility : public Dispatchable {
public:
  static FOURCC getServiceType() { return WaSvc::ACCESSIBILITY; }

  Accessible *createAccessibleObject(RootWnd *w);

  enum {
    SVC_ACCESSIBILITY_CREATEACCESSIBLEOBJECT=10,
  };
};

inline Accessible *svc_accessibility::createAccessibleObject(RootWnd *w) {
  return _call(SVC_ACCESSIBILITY_CREATEACCESSIBLEOBJECT, (Accessible *)NULL, w);
}

class NOVTABLE svc_accessibilityI: public svc_accessibility {
  public:
    virtual Accessible *createAccessibleObject(RootWnd *w)=0;

  protected:
    RECVS_DISPATCH;
};

#include "servicei.h"
template <class T>
class AccessibilityCreatorSingle : public waServiceFactoryTSingle<svc_accessibility, T> {
public:
  svc_accessibility *getHandler() {
    return getSingleService();
  }
};


#endif
