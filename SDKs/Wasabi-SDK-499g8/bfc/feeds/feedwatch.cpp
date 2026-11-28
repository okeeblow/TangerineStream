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

//<?#include "<class data="implementationheader"/>"
#include "feedwatch.h"
//?>

#include "../../studio/services/svc_textfeed.h"

class FeedWatcher::FWDV : public DependentViewerI {
public:
  friend class FeedWatcher;

  FWDV(FeedWatcher *parent);

  virtual int viewer_onItemDeleted(Dependent *item);
  virtual int viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param, void *ptr, int ptrlen);

  FeedWatcher *parent;
};

FeedWatcher::FWDV::FWDV(FeedWatcher *_parent) : parent(_parent) { }

int FeedWatcher::FWDV::viewer_onItemDeleted(Dependent *item) {
  return parent->fwdv_onItemDeleted(item);
}

int FeedWatcher::FWDV::viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param, void *ptr, int ptrlen) {
  return parent->fwdv_onEvent(item, event, param, ptr, ptrlen);
}

FeedWatcher::FeedWatcher() :
  registered_syscb(0), textfeed(NULL)
{
  fwdv = new FWDV(this);
}


FeedWatcher::~FeedWatcher() {
  releaseFeed();
  if (registered_syscb) {
    WASABI_API_SYSCB->syscb_deregisterCallback(static_cast<SvcCallbackI*>(this));
    DebugString("unreg syscb");
  }
  delete fwdv;
}

int FeedWatcher::setFeed(const char *feedid) {
  if (!registered_syscb++) WASABI_API_SYSCB->syscb_registerCallback(static_cast<SvcCallbackI*>(this));

  releaseFeed();

  feed_id = feedid;
  if (!feed_id.isempty()) textfeed = TextFeedEnum(feed_id).getFirst();

  if (textfeed != NULL) {
    fwdv->viewer_addViewItem(textfeed->getDependencyPtr());
    feedwatcher_onSetFeed(textfeed);
//CUT    if (first_cb) feedwatcher_onFeedChange(textfeed->getFeedText(feed_id));
  }

  return 1;
}

void FeedWatcher::releaseFeed() {
  if (textfeed) { 
    fwdv->viewer_delViewItem(textfeed->getDependencyPtr());
    feed_id = "";
    SvcEnum::release(textfeed);
    textfeed = NULL;
  }                   
}

const char *FeedWatcher::getFeedId() {
  return feed_id;
}

int FeedWatcher::fwdv_onItemDeleted(Dependent *item) {
  textfeed = NULL;
  // send text change msg? dunno for sure
  return 1;
}

int FeedWatcher::fwdv_onEvent(Dependent *item, int event, int param2, void *ptr, int ptrlen) {
  if (textfeed && item == textfeed->getDependencyPtr() && event == svc_textFeed::Event_TEXTCHANGE && STRCASEEQLSAFE((const char *)param2, feed_id)) {
    feedwatcher_onFeedChange((const char *)ptr);
    return 1;
  }
  return 0;
}

void FeedWatcher::svccb_onSvcRegister(FOURCC type, waServiceFactory *svc) {
  // brand new feed and we don't have one, try to register it
//CUT __asm int 3;
  if (type == WaSvc::TEXTFEED && textfeed == NULL) {
    if (!feed_id.isempty()) {
      String tmp(feed_id);
      setFeed(tmp);
    }
  }
}
