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
#ifndef _VIDEO_OVERLAY_H
#define _VIDEO_OVERLAY_H

#include <ddraw.h>
#include <multimon.h>
#include "videoout.h"

class OverlayVideoOutput : public VideoOutput {
public:
  OverlayVideoOutput();
  virtual ~OverlayVideoOutput();

  int create(VideoWnd *parent, int w, int h, const char *type, int flipit, float aspectratio); //return 1 if ok
  int needChange() { return needchange; }

  int onPaint(Canvas *canvas, RECT &r);
  void displayFrame(const char *buf, int size, int time);

  void goFullScreen();
  void removeFullScreen();
  int showOSD();
  void hideOSD();

  void timerCallback();

  GUID m_devguid;
  int m_found_devguid;
  HMONITOR m_monitor_to_find;
  int m_mon_x, m_mon_y;

private:
  int width, height, flip;
  int needchange;
  String type;
  VideoWnd *m_parent;
  LPDIRECTDRAW		   lpDD;
  LPDIRECTDRAWSURFACE lpddsOverlay, lpddsPrimary;
  DDCAPS          capsDrv;
  unsigned int    uDestSizeAlign, uSrcSizeAlign;
  DWORD           dwUpdateFlags;
  DDOVERLAYFX     ovfx;
  RECT rs,rd;
  RECT m_oldrd;

  bool initing;
  int is_fullscreen, yuy2_output, uyvy_output;

  GuiObjectWnd *fullscreen_controls;
  
  void getRects(RECT *drs, RECT *drd);
};

#endif