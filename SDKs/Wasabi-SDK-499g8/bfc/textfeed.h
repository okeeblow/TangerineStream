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
#ifndef _TEXTFEED_H
#define _TEXTFEED_H

#include "../common/common.h"
#include "map.h"
#include "depend.h"
#include "../studio/services/svc_textfeed.h"

/**
  This is the standard helper class for implementing a textfeed.
  Be sure to check out class EzFeed in ezfeed.h, which combines this
  class with a service factory.
*/
class COMEXP TextFeed : public svc_textFeedI, public DependentI {
public:
/**
  Call this to register your feeds by id. Make the ids unique!
  @see sendFeed()
  @ret TRUE if succeeded, FALSE on error (i.e. nonunique id)
*/
  int registerFeed(const char *feedid, const char *initial_text="", const char *description="");

/**
  Call this to send text into a feed.
  @see registerFeed()
  @ret TRUE if succeeded, FALSE on error (i.e. feedid not registered)
*/
  int sendFeed(const char *feedid, const char *text);

/**
  Gives the most recently sent text on a feed.
*/
  virtual const char *getFeedText(const char *feedid);

/**
  Gives a description for the feed (used by accessibility).
*/
  virtual const char *getFeedDescription(const char *feedid);

protected:
  virtual Dependent *getDependencyPtr() { return this; }
  virtual void dependent_onRegViewer(DependentViewer *viewer, int add);
  virtual void *dependent_getInterface(const GUID *classguid);

/**
  Called when someone subscribes to this feed.
*/
  virtual void onRegClient() { }
  virtual void onDeregClient() { }

  virtual int hasFeed(const char *name);

private:
  Map<String, Pair<String, String> > feeds;
};

#endif
