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

#include "autofeed.h"
#include "../../common/urlgrab.h"
#include "../util/tmpnamestr.h"

enum {
  TIMER_BEGIN=1,
  TIMER_FETCH,
};

AutoXmlFeed::AutoXmlFeed(const char *_url, const char *_xmlprefix, const char *_feedidprefix, int interval, bool _gxmlcb)
: url(_url), xmlprefix(_xmlprefix), feedidprefix(_feedidprefix)
{
  maxfetch = nfetch = 0;
  grab = NULL;
  globalXmlCb = _gxmlcb;
  timerclient_setTimer(TIMER_BEGIN, interval);
  timerclient_setTimer(TIMER_FETCH, 100);
  timerclient_timerCallback(TIMER_BEGIN);	// go!

  WASABI_API_SVC->service_register(this);
}

AutoXmlFeed::~AutoXmlFeed() {
  WASABI_API_SVC->service_deregister(this);

  if (grab != NULL) {
    UNLINK(grab->getFilename());
    delete grab;
  }
  timerclient_killTimer(TIMER_BEGIN);
  timerclient_killTimer(TIMER_FETCH);
}

void AutoXmlFeed::setMaxFetches(int n) {
  maxfetch = n;
}

void AutoXmlFeed::setNumFetches(int n) {
  nfetch = n;
}

int AutoXmlFeed::hasFeed(const char *name) {
  // say "yes" if it's our special prefix
  if (!feedidprefix.isempty() && !STRNINCMP(name, feedidprefix)) return TRUE;
  return EzFeed::hasFeed(name);
}

void AutoXmlFeed::timerclient_timerCallback(int id) {
  if (id == TIMER_FETCH) {
    if (grab == NULL) return;
    int r = grab->run();
    if (r == 0) return;

    if (r > 0) {
      nfetch++;
//CUT  DebugString("done grabo for %s, read xml", grab->getUrl());
      String cbt = xmlprefix;
      cbt += "/*";
//CUTDebugString("Registercallback for '%s'", cbt.v());
      XmlReaderCallbackI * cb = this;
      if(globalXmlCb) cb = NULL;
      WASABI_API_XML->xmlreader_registerCallback(cbt, this);
      WASABI_API_XML->xmlreader_loadFile(grab->getFilename(), cb);
      WASABI_API_XML->xmlreader_unregisterCallback(this);

      // apply map of values
      for (int i = 0; i < map.getNumItems(); i++) {
        String feedid = map.enumIndexByPos(i, String());
        String str = map.enumItemByPos(i, String());
        if (!registerFeed(feedid, str)) sendFeed(feedid, str);
//CUT    DebugString("APLY %s = '%s'", feedid.v(), str.v());
      }

      map.deleteAll();
    }
//CUT  DebugString("*** done %s", grab->getUrl());
    UNLINK(grab->getFilename());
    delete grab; grab = NULL;
  } else if (id == TIMER_BEGIN) {
    if (grab == NULL) {
      if (maxfetch == 0 || nfetch < maxfetch)
        grab = new UrlGrab(url, TmpNameStr());
//CUT  DebugString("make grabo for %s", url);
    }
  }
}

void AutoXmlFeed::xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params) {
}

void AutoXmlFeed::xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *str) {
  String s(str);
  s.trim();
  if (!s.isempty()) {
    String feedidsuffix(xmltag);
    String path(xmlpath);

    if(xmlpath != xmlprefix) {
      path.replace(xmlprefix.getValue(), "");
      if(path.getChar(0) == '/') path.lSplitChar('/');
    }

    // make everything a feed eh
    String feedid = StringPrintf("%s:%s", feedidprefix.v(), path);
    String val;
    int r = map.getItem(feedid, &val);
    if (r) val += ";";	// was there, needs a ";"
    val += s;
    map.setItem(feedid, val);

#if defined(_DEBUG)
    DebugString("%s = '%s'", feedid.v(), val.v());
#endif

  }
}
