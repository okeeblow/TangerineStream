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
#include "normal.h"
#include "cnv.h"

#include <studio/services/servicei.h>
#include <bfc/depview.h>
#include <bfc/attribs/attribs.h>
#include <bfc/textfeed.h>

static WACnormal wac;
WAComponentClient *the = &wac;

// {050D1FD4-3970-4c75-86AC-2D4FEADDE755}
static const GUID WACnormalguid = 
{ 0x50d1fd4, 0x3970, 0x4c75, { 0x86, 0xac, 0x2d, 0x4f, 0xea, 0xdd, 0xe7, 0x55 } };

_bool learn("Autonormalize", TRUE);

int learn_now;

class MyFeed : public TextFeed {
public:
  static const char *getServiceName() { return "Audio normalizer status"; }
  MyFeed() {
    registerFeed("Normalizer:STATUS", "Audio normalizer idle GRONK!");
  }
};

TextFeedCreatorSingle<MyFeed> feedfac;

void sendStatus(const char *st) {
  feedfac.getSingleService()->sendFeed("Normalizer:STATUS", st);
}

class Learnin : public DependentViewerTPtr<CfgItem> {
public:
  Learnin() {
//CUT    // {B66F7CE8-8BB9-4daf-95F5-4C45B05CF1B7}
//CUT    const GUID guid = 
//CUT    { 0xb66f7ce8, 0x8bb9, 0x4daf, { 0x95, 0xf5, 0x4c, 0x45, 0xb0, 0x5c, 0xf1, 0xb7 } };
    CfgItem *cfgitem = WASABI_API_CONFIG->config_getCfgItemByGuid(WACnormalguid);
    if (cfgitem != NULL) {
      learn_now = cfgitem->getDataAsInt("Autonormalize");
      viewer_addViewItem(cfgitem);
    }
  }
  virtual int viewer_onEvent(CfgItem *item, int event, int param, void *ptr, int ptrlen) {
    if (event == CfgItem::Event_ATTRIBUTE_CHANGED && STRCASEEQLSAFE((const char *)ptr, "Autonormalize")) {
      learn_now = item->getDataAsInt("Autonormalize");
      return 1;
    }
    return 0;
  }
};

static Learnin *learnio;

WACnormal::WACnormal() : WAComponentClient("Audio Normalizer") {
  registerService(new MediaConverterCreator<Analyzer>);
  registerService(&feedfac);
  registerAttribute(&learn);
}

WACnormal::~WACnormal() {
}

GUID WACnormal::getGUID() {
  return WACnormalguid;
}

void WACnormal::onCreate() {
  learnio = new Learnin;
}

void WACnormal::onDestroy() {
  delete learnio; learnio = NULL;
}

int WACnormal::onNotify(int cmd, int param1, int param2, int param3, int param4) {
  switch (cmd) {
    case WAC_NOTIFY_SERVICE_NOTIFY:
      switch (param1) { 
        case SvcNotify::ONAPPRUNNING:
          pimpo = new Pumpo();
        break;
        case SvcNotify::ONSHUTDOWN:
          delete pimpo; pimpo = NULL;
        break;
      }
    break;
  }
  return 0;
}
