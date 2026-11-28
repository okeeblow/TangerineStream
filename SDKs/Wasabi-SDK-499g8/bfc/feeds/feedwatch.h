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
#ifndef __FEEDWATCH_H
#define __FEEDWATCH_H

#include <bfc/std.h>
#include <studio/svccb.h>

class svc_textFeed;
class Dependent;

class COMEXP FeedWatcher : private SvcCallbackI {
public:
  FeedWatcher();
  virtual ~FeedWatcher();

  int setFeed(const char *feedid);
  void releaseFeed();

  const char *getFeedId();

  virtual void feedwatcher_onSetFeed(svc_textFeed *svc) { }
  virtual void feedwatcher_onFeedChange(const char *data) { }

protected:
  class FWDV;
  friend class FWDV;
  // catches text feed change events
  virtual int fwdv_onItemDeleted(Dependent *item);
  virtual int fwdv_onEvent(Dependent *item, int event, int param2, void *ptr, int ptrlen);

private:
  // catches new feeds being registered
  virtual void svccb_onSvcRegister(FOURCC type, waServiceFactory *svc);

  int registered_syscb;
  String feed_id;
  svc_textFeed *textfeed;
  FWDV *fwdv;
};

#endif
