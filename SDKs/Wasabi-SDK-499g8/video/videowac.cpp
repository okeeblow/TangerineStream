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

#include "videowac.h"
#include "video.h"
#include "crop.h"
#include "resource.h"

#include <bfc/wnds/buttwnd.h>
#include <bfc/attribs/attribs.h>
#include <bfc/wndcreator.h>
#include <studio/services/creators.h>

#include "wndcreatesvc.h"
#include "videownd.h"


static WACNAME wac;
WAComponentClient *the = &wac;
WACNAME *videowac = &wac;

// {DF4508FF-8A9F-46a4-B7CB-3EEAF46CA4ED}
static const GUID cfg_video_guid = 
{ 0xdf4508ff, 0x8a9f, 0x46a4, { 0xb7, 0xcb, 0x3e, 0xea, 0xf4, 0x6c, 0xa4, 0xed } };

// {F0816D7B-FFFC-4343-80F2-E8199AA15CC3}
static const GUID guid = 
{ 0xf0816d7b, 0xfffc, 0x4343, { 0x80, 0xf2, 0xe8, 0x19, 0x9a, 0xa1, 0x5c, 0xc3 } };

_bool vid_aspectadj("Maintain aspect ratio", true);
_bool vid_vsync("Sync to refresh", false);
_bool vid_overlays("Allow overlays", true);
_bool vid_ddraw("Allow fast blts", true);
_bool vid_resizewnd("Auto window resize", true);
_bool vid_clickpause("Stop and start video on left click", false);
_bool vid_stopvid("Stop playback on window close", true);
_bool vid_forcevflip("Force vertical flip", false);
_bool vid_crop169("Crop 4:3 to 16:9", false);
_bool vid_cropdim("Change dimensions when cropping", true);
_bool vid_noss("Disable screen saver in fullscreen", true);
_int vid_hidetimer("Hide cursor interval (ms)", 2000);
_bool vid_applyforced("Force default aspect ratios", true);
_string vid_forced("Default aspect ratios", "720x480=1.125;352x240=1.1;352x480=0.55");
_bool vid_support_yuy2("Accept YUY2", TRUE);
_bool vid_support_uyvy("Accept UYVY", TRUE);
_bool vid_support_yv12("Accept YV12", FALSE);

WACNAME::WACNAME() : WAComponentClient("Video") {
  registerAttribute(&vid_aspectadj);
  registerAttribute(&vid_vsync);
  registerAttribute(&vid_overlays);  
  registerAttribute(&vid_ddraw);
  registerAttribute(&vid_resizewnd);
  registerAttribute(&vid_clickpause);
  registerAttribute(&vid_stopvid);
  registerAttribute(&vid_forcevflip);
  registerAttribute(&vid_crop169);
  registerAttribute(&vid_cropdim);
  registerAttribute(&vid_noss);
  registerAttribute(&vid_hidetimer);
  registerAttribute(&vid_applyforced);
  registerAttribute(&vid_forced);
  registerAttribute(&vid_support_yuy2);
  registerAttribute(&vid_support_uyvy);
  registerAttribute(&vid_support_yv12);

  registerService(new MediaConverterCreator<Video>);
  registerService(new MediaConverterCreator<Crop>);
  registerService(new WndCreateCreator<VideoWndCreateSvc>);
  registerService(new WndCreateCreatorSingle< CreateBucketItem<WACNAME> >);
  registerService(new ActionCreator<VideoActionsSvc>);
  registerSkinFile("xml/video.xml");

  registerPrefGroup("winamp.preferences.video", "Video", cfg_video_guid);
}

GUID WACNAME::getGUID() {
  return guid;
}

GUID WACNAME::getPrefsGUID() {
  return cfg_video_guid;
}

void WACNAME::onCreate() {
  api->autopopup_registerGuid(videowac->getGUID(), "Video");
}


int WACNAME::onCommand(const char *cmd, int param1, int param2, void *ptr, int ptrlen) {
  if (STRCASEEQL(cmd,"displaywindow")) {
    api->skinwnd_createByGuid(videowac->getGUID());
    return 1;
  }
  else if(STRCASEEQL(cmd,"endvideo")) {
    if(videownd) videownd->endVideo();
    return 1;
  }
  else if(STRCASEEQL(cmd,"resizewindow")) {
    RootWnd *p=(RootWnd *)ptr;
    RECT *rect=(RECT *)param1;
    p->resize(rect->left,rect->top,rect->right,rect->bottom);
    return 1;
  }
  /*else if (!strcmp(cmd, "fullscreen")) videownd->goFullScreen();
  else if (!strcmp(cmd, "scale1x")) videownd->scale(1);
  else if (!strcmp(cmd, "scale2x")) videownd->scale(2);
  if (!strcmp(cmd, "scale3x")) videownd->scale(3);
  if (!strcmp(cmd, "scale4x")) videownd->scale(4);
  // BU quick hack for watching TV :)
  else if (!STRNINCMP(cmd, "skip")) {
    int amt = ATOI(cmd + 5);
    if (amt != 0) {
      int curpos = api->core_getPosition(0);
      curpos += amt * 1000;
      if(curpos<0) curpos=0;
      api->core_setPosition(0, curpos);
    }
  }*/
  return 0;
}


int WACNAME::onNotify(int cmd, int param1, int param2, int param3, int param4)
{
/*  switch(cmd) {
  case WAC_NOTIFY_DEACTIVATE:
    if(vid_stopvid) Video::stopPlayback();
    break;
  }*/
  return 0;
}


const char *WACNAME::getWindowTypeName() { return "Video Window"; }
GUID WACNAME::getWindowTypeGuid() { return guid; }


void WACNAME::setIconBitmaps(ButtonWnd *button) {
  button->setBitmaps(videowac->gethInstance(), IDB_TAB_NORMAL, NULL, IDB_TAB_HILITED, IDB_TAB_SELECTED);
}
