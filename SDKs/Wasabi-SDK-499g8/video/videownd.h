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
#ifndef __VIDEOWND_H
#define __VIDEOWND_H

#include <common/guiobjwnd.h>
#include <bfc/bitmap.h>
#include <common/critsec.h>
#include <bfc/appcmds.h>
#include <core/corecb.h>

#include "videoout.h"
#include "vid_overlay.h"
#include "vid_ddraw.h"

#define VIDEOWND_PARENT GuiObjectWnd

class VideoWnd : public VIDEOWND_PARENT, AppCmdsI, CoreCallbackI {
public:
  VideoWnd();
  virtual ~VideoWnd();

  int onPaint(Canvas *canvas);
  int onInit();
  int onResize();

  virtual int onKeyDown(int keyCode);
  virtual int onLeftButtonDown(int x, int y);
  virtual int onLeftButtonDblClk(int x, int y);
  virtual int onRightButtonDown(int x, int y);
  virtual int onMouseWheelUp(int click, int lines);
  virtual int onMouseWheelDown(int click, int lines);

  virtual void onEnterArea();
  virtual void onLeaveArea();

  virtual void timerCallback(int id);

  int setSize(int w, int h, const char *type, int flipit, float aspectratio);
  void endVideo();
  void addFrame(const char *buf, int size, int time);
  void adjustAspect(RECT &rd);
  void setPalette(RGBQUAD *pal) { 
    if(m_video_output) m_video_output->setPalette(pal);
  }
  
  void scale(float ratio);

  int isFullScreen();
  void goFullScreen();
  void removeFullScreen();
  void toggleFullScreen();

  void openMiscMenu();
  void openVideoOptions();

  void jump(int seconds);

  virtual int handleTransparency() { return 0; } // no transparency for me please, i need to overlay
    
  virtual int wantDoubleClicks() { return 1; }

  virtual void appcmds_onCommand(int id, const RECT *buttonRect, int which_btn);

  virtual void onSetVisible(int show);
  
  virtual int onAcceleratorEvent(const char *name);

  enum {
    CMD_FULLSCREEN = 0,
    CMD_SCALE,
    CMD_BACK7S,
    CMD_SKIP30S,
  };

  int kickOSD();

protected:
#ifdef WIN32
  virtual LRESULT wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
// corecb
  virtual int corecb_onStarted() { return kickOSD(); }
  virtual int corecb_onSeeked(int newpos) { return kickOSD(); }
  virtual int corecb_onTitleChange(const char *title) { return kickOSD(); }

  VideoOutput *createVideoOutput(int n);

private:
  float aspect;
  int width, height, flip;
  int quit;
  String type;
  int uyvy_output,yuy2_output;
  int use_overlay;
  int is_fullscreen;
  HWND oldfsparent;
  RECT oldfsrect;
  LONG oldfsstyle;

  CriticalSection videocs;

  int hidecursor;
  HWND prev_above;

  VideoOutput *m_video_output;
};

extern VideoWnd *videownd;

// YV12 needs to pass a YV12_PLANES structure.
typedef	struct {
	unsigned char*	baseAddr;
	long			rowBytes;
} YV12_PLANE;

typedef	struct {
	YV12_PLANE	y;
	YV12_PLANE	u;
	YV12_PLANE	v;
} YV12_PLANES;

#include "../studio/services/svc_action.h"

class VideoActionsSvc : public svc_actionI {
public:
  VideoActionsSvc();

  static const char *getServiceName() { return "Video Actions Service"; }

  virtual int onActionId(int actionid, const char *action, const char *param=NULL, int p1=0, int p2=0, void *data=NULL, int datalen=0, RootWnd *source=NULL);

};


#endif
