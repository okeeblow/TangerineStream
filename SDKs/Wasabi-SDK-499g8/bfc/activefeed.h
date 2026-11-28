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
#ifndef __ACTIVEFEED_H
#define __ACTIVEFEED_H

#include "../studio/services/servicei.h"
#include "../studio/services/svc_action.h"
#include "../studio/services/svc_textfeed.h"
#include "textfeed.h"

class ActiveFeedFeedCreatorSingle;

class ActiveFeed : public svc_actionI {
private:
  enum { ACTIVEFEED = 0x0ACFEED0 };
public:
  class ActiveFeedFeed : public TextFeed {
  public:
    static const char *getServiceName();
  };
public:
  ActiveFeed();
  virtual int onActionId(int pvtid, const char *action, const char *param=NULL, int p1=0, int p2=0, void *data=NULL, int datalen=0, RootWnd *source=NULL);
  void setActiveFeedFeedCreator(ActiveFeedFeedCreatorSingle *_feed);
  static const char *getServiceName();
  void registerActiveFeed(const char *feedid, const char *text, const char *description="");
  void sendFeed(const char *feedid, const char *text);
private:
  ActiveFeedFeedCreatorSingle *feed;
};

class ActiveFeedFeedCreatorSingle : public waServiceFactoryTSingle<svc_textFeed, ActiveFeed::ActiveFeedFeed> {
public:
  ActiveFeed::ActiveFeedFeed *getFeed();
  void sendFeed(const char *feedid, const char *text);
  void registerFeed(const char *feedid, const char *description="");
};

class ActiveFeedCreatorSingle : public waServiceFactoryTSingle<svc_action, ActiveFeed> {
public:
  ActiveFeed *getHandler();
  void registerActiveFeed(const char *feedid, const char *text, const char *description="");
  void sendFeed(const char *feedid, const char *text);
  void setActiveFeedFeedCreator(ActiveFeedFeedCreatorSingle *_feed);
};

#if 0
// Usage:

static ActiveFeedCreatorSingle myActiveFeed;
static ActiveFeedFeedCreatorSingle myAFF;

WACMyWac::WACMyWac() : WAComponentClient("My Wac") {
  registerService(&myActiveFeed);
  registerService(&myAFF);
  myActiveFeed.setActiveFeedFeedCreator(&myAFF);
}
WACMyWac::onCreate() {
  // Register the feedids you want to make active with the system.
  myActiveFeed.registerActiveFeed("FeedID1","The initial text I wish to display.");
}
void someOtherFunctionLaterOn() {
  FireAction("FeedID1","No, wait, I changed my mind, say this now.");
}

#endif

#endif//__ACTIVEFEED_H
