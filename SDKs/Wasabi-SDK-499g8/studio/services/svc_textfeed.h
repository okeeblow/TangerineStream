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
#ifndef _SVC_TEXTFEED_H
#define _SVC_TEXTFEED_H

#include "../../bfc/dispatch.h"
#include "services.h"
#include "../../bfc/string.h"

class Dependent;

class NOVTABLE svc_textFeed : public Dispatchable {
public:
  static FOURCC getServiceType() { return WaSvc::TEXTFEED; }
  static const GUID *depend_getClassGuid() {
    // {A04E3420-CBA1-4ae1-B3C0-8DE127D2B861}
    static const GUID ret = 
    { 0xa04e3420, 0xcba1, 0x4ae1, { 0xb3, 0xc0, 0x8d, 0xe1, 0x27, 0xd2, 0xb8, 0x61 } };
    return &ret;
  }
  static const char *getServiceName() { return "Untitled Textfeed"; }
  
  int hasFeed(const char *name);
  const char *getFeedText(const char *name);
  const char *getFeedDescription(const char *name);

  Dependent *getDependencyPtr();

  enum {
    Event_TEXTCHANGE=100, // param is const char* to id, ptr points to new text
  };

  enum {
    SVCTEXTFEED_HASFEED=10,
//20,30 retired
    SVCTEXTFEED_GETFEEDTEXT=40,
    SVCTEXTFEED_GETFEEDDESC=45,
    SVCTEXTFEED_GETDEPENDENCYPTR=100,
  };

};

inline int svc_textFeed::hasFeed(const char *name) {
  return _call(SVCTEXTFEED_HASFEED, 0, name);
}

inline const char *svc_textFeed::getFeedText(const char *name) {
  return _call(SVCTEXTFEED_GETFEEDTEXT, (const char *)NULL, name);
}

inline const char *svc_textFeed::getFeedDescription(const char *name) {
  return _call(SVCTEXTFEED_GETFEEDDESC, (const char *)NULL, name);
}

inline Dependent *svc_textFeed::getDependencyPtr() {
  return _call(SVCTEXTFEED_GETDEPENDENCYPTR, (Dependent*)NULL);
}

class NOVTABLE svc_textFeedI : public svc_textFeed {
public:
  virtual int hasFeed(const char *name)=0;
  virtual const char *getFeedText(const char *name)=0;
  virtual const char *getFeedDescription(const char *name)=0;
  virtual Dependent *getDependencyPtr()=0;
  virtual void *dependent_getInterface(const GUID *classguid);//implemented for you

protected:
  RECVS_DISPATCH;
};

// see helper class TextFeed

#include "servicei.h"
template <class T>
class TextFeedCreatorSingle : public waServiceFactoryTSingle<svc_textFeed, T> {
public:
  svc_textFeed *getFeed() {
    return getSingleService();
  }
};

#include "../../bfc/svc_enum.h"

class TextFeedEnum : public SvcEnumT<svc_textFeed> {
public:
  TextFeedEnum(const char *_feedid) : feedid(_feedid) {}
protected:
  virtual int testService(svc_textFeed *svc) {
    return (svc->hasFeed(feedid));
  }
private:
  String feedid;
};

#endif
