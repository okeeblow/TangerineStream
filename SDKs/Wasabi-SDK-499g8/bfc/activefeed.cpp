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
#include "precomp.h"
#include "activefeed.h"


const char *ActiveFeed::ActiveFeedFeed::getServiceName() { 
  return "ActiveFeedFeed"; 
}

ActiveFeed::ActiveFeed() {
  feed = NULL;
}

int ActiveFeed::onActionId(int pvtid, const char *action, const char *param, int p1, int p2, void *data, int datalen, RootWnd *source) {
  if (pvtid == ACTIVEFEED) {
    if (param) {
      sendFeed(action, param);
    } else {
      sendFeed(action, "");
    }
    return 1;
  }
  return 0;
}           

void ActiveFeed::setActiveFeedFeedCreator(ActiveFeedFeedCreatorSingle *_feed) { 
  feed = _feed; 
}

const char *ActiveFeed::getServiceName() { 
  return "ActiveFeed"; 
}

void ActiveFeed::registerActiveFeed(const char *feedid, const char *text, const char *description) {
  ASSERT(feed);
  registerAction(feedid, ACTIVEFEED);
  feed->registerFeed(feedid);
  feed->sendFeed(feedid, text);
}

void ActiveFeed::sendFeed(const char *feedid, const char *text) {
  ASSERT(feed);
  feed->sendFeed(feedid, text);
}

ActiveFeed::ActiveFeedFeed *ActiveFeedFeedCreatorSingle::getFeed() {
  return getSingleService();
}
void ActiveFeedFeedCreatorSingle::sendFeed(const char *feedid, const char *text) {
  getFeed()->sendFeed(feedid, text);
}
void ActiveFeedFeedCreatorSingle::registerFeed(const char *feedid, const char *description) {
  getFeed()->registerFeed(feedid, "", description);
}

ActiveFeed *ActiveFeedCreatorSingle::getHandler() {
  return getSingleService();
}

void ActiveFeedCreatorSingle::registerActiveFeed(const char *feedid, const char *text, const char *description) {
  getHandler()->registerActiveFeed(feedid, text, description);
}

void ActiveFeedCreatorSingle::sendFeed(const char *feedid, const char *text) {
  getHandler()->sendFeed(feedid, text);
}

void ActiveFeedCreatorSingle::setActiveFeedFeedCreator(ActiveFeedFeedCreatorSingle *_feed) { 
  getHandler()->setActiveFeedFeedCreator(_feed);
}

