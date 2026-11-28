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
#ifndef _SVC_XUIOBJECT_H
#define _SVC_XUIOBJECT_H

#include "../../bfc/dispatch.h"
#include "services.h"

class GuiObject;
class XmlReaderParams;

class svc_xuiObject : public Dispatchable {
public:
  static FOURCC getServiceType() { return WaSvc::XUIOBJECT; }
  int testTag(const char *xmltag);
  GuiObject *instantiate(const char *xmltag, XmlReaderParams *params=NULL);
  void destroy(GuiObject *g);

  enum {
    XUI_TESTTAG=10,
    //XUI_INSTANTIATE=20, // RETIRED
    XUI_INSTANTIATEWITHPARAMS=25,
    XUI_DESTROY=30,
  };
};

inline int svc_xuiObject::testTag(const char *xmltag) {
  return _call(XUI_TESTTAG, 0, xmltag);
}

inline GuiObject *svc_xuiObject::instantiate(const char *xmltag, XmlReaderParams *params) {
  return _call(XUI_INSTANTIATEWITHPARAMS, (GuiObject *)NULL, xmltag, params);
}

inline void svc_xuiObject::destroy(GuiObject *o) {
  _voidcall(XUI_DESTROY, o);
}

// derive from this one
class svc_xuiObjectI : public svc_xuiObject {
public:
  virtual int testTag(const char *xmltag)=0;
  virtual GuiObject *instantiate(const char *xmltag, XmlReaderParams *params=NULL)=0;
  virtual void destroy(GuiObject *o)=0;

protected:
  RECVS_DISPATCH;
};

#include "servicei.h"
template <class T>
class XuiObjectCreator : public waServiceFactoryTSingle<svc_xuiObject, T> {
public:
  virtual const char *svc_getTestString() { return T::xuisvc_getXmlTag(); }
};

#include "../../bfc/svc_enum.h"
#include "../../bfc/string.h"

class XuiObjectSvcEnum : public SvcEnumT<svc_xuiObject> {
public:
  XuiObjectSvcEnum(const char *xmltag) : tag(xmltag) {}
protected:
  virtual int testService(svc_xuiObject *svc) {
    return (svc->testTag(tag));
  }
private:
  String tag;
};

#endif
