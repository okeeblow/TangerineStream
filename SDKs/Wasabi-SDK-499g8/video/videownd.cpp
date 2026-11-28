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

#include "videownd.h"
#include "videowac.h"
#include "video.h"

#include <windows.h>

#include <bfc/bitmap.h>
#include <bfc/canvas.h>
#include <bfc/attribs/attribs.h>
#include <core/buttons.h>
#include <common/popup.h>

extern _bool vid_aspectadj;
extern _bool vid_overlays;
extern _bool vid_ddraw;
extern _bool vid_resizewnd;
extern _bool vid_clickpause;
extern _bool vid_forcevflip;
extern _bool vid_vsync;
extern _bool vid_stopvid;
extern _int vid_hidetimer;

#define TIMER_HIDECURSOR 100

VideoWnd *videownd=NULL;

VideoWnd::VideoWnd() {
  setName("Video");
  quit=0;
  flip=0;
  width=height=-1;
  use_overlay=0;
  is_fullscreen=0;
  type="";
  setVirtual(0);
  registerAcceleratorSection("video");
  hidecursor = 0;
  prev_above = NULL;

  m_video_output=NULL;
}

VideoWnd::~VideoWnd() {
  quit=1;
  if(vid_stopvid) Video::stopPlayback();
  videownd=NULL;
  endVideo();
  killTimer(1);
  getGuiObject()->guiobject_removeAppCmds(this);
  api->core_delCallback(0, this);
}

#ifdef WIN32
LRESULT VideoWnd::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_SETCURSOR:
      if (is_fullscreen) {
        SetCursor(hidecursor ? NULL : LoadCursor(NULL, IDC_ARROW));
        return TRUE;    
      }
    break;

    case WM_MOUSEMOVE:
      kickOSD();
    break;

    case WM_SYSCOMMAND:
      // eat screen saver message when fullscreen
      extern _bool vid_noss;
      if (((wParam & 0xfff0) == SC_SCREENSAVE || (wParam & 0xfff0) == SC_MONITORPOWER) && is_fullscreen && vid_noss) return TRUE;
    break;
  }
  return VIDEOWND_PARENT::wndProc(hWnd,uMsg,wParam,lParam);
}
#endif

int VideoWnd::onResize() {
  return VIDEOWND_PARENT::onResize();
}

int VideoWnd::onInit() {
  VIDEOWND_PARENT::onInit();
  setTip("Video window");
  setTimer(1,100);

  appcmds_addCmd("Fullscreen", CMD_FULLSCREEN);
  appcmds_addCmd("Size", CMD_SCALE);
  appcmds_addCmd("Back 7s", CMD_BACK7S, SIDE_RIGHT);
  appcmds_addCmd("Skip 30s", CMD_SKIP30S, SIDE_RIGHT);

  getGuiObject()->guiobject_addAppCmds(this);

  api->core_addCallback(0, this);

  return 1;
}

void VideoWnd::appcmds_onCommand(int id, const RECT *buttonRect, int which_btn) {
  switch (id) {
    case CMD_FULLSCREEN:
      goFullScreen();
    break;
    case CMD_SCALE: {
      PopupMenu pop(this);
      pop.addCommand("50%", 50);
      pop.addCommand("100%", 100);
      pop.addCommand("150%", 150);
      pop.addCommand("200%", 200);
      pop.addCommand("400%", 400);
      int r = pop.popAtXY(buttonRect->left, buttonRect->bottom);
      if (r > 0) scale((float)r / 100.f);
    }
    break;
    case CMD_BACK7S:
      jump(-7);
    break;
    case CMD_SKIP30S:
      jump(30);
    break;
  }
}

void VideoWnd::jump(int seconds) {
  if (seconds != 0) {
    int curpos = api->core_getPosition(0);
    curpos += seconds * 1000;
    if(curpos<0) curpos=0;
    api->core_setPosition(0, curpos);
  }
}

void VideoWnd::adjustAspect(RECT &rd)
{
  if (vid_aspectadj)
  {
    int outh=rd.bottom-rd.top;
    int outw=rd.right-rd.left;

    int newh=(int)((aspect*height*outw)/(double)width);
    int neww=(int)((width*outh)/(height*aspect));

    if (outh > newh) // black bars on top and bottom
    {
      int d=outh - newh;
      rd.top+=d/2;
      rd.bottom-=d-d/2;
    }
    else if (outw > neww) // black bars on left and right
    {
      int d=outw - neww;
      rd.left+=d/2;
      rd.right-=d-d/2;
    }
  }
}

void VideoWnd::timerCallback(int id) {
  if(id==1) {
    if(!quit && m_video_output) m_video_output->timerCallback();
  } else if (id == TIMER_HIDECURSOR) {
    killTimer(TIMER_HIDECURSOR);
    if (is_fullscreen) {	// hide cursor and controls
      SetCursor(NULL);
      hidecursor = 1;
      if(m_video_output) m_video_output->hideOSD();
      setFocus();
    }
  }
}


int VideoWnd::onPaint(Canvas *canvas) {
  PaintCanvas paintcanvas;

  if (canvas == NULL) {
    if (!paintcanvas.beginPaint(this)) return 0;
    canvas = &paintcanvas;
  }

  RECT ocr = clientRect();
  if (!m_video_output || !m_video_output->onPaint(canvas,ocr)) {
    canvas->fillRect(&ocr, RGB(0,0,0));
  }
  return 0;
}													  

int VideoWnd::onKeyDown(int keyCode) {
  if(keyCode==27 && is_fullscreen) removeFullScreen();
  else if(keyCode==49 && !is_fullscreen) scale(0.5f);
  else if(keyCode==50 && !is_fullscreen) scale(1.0f);
  else if(keyCode==51 && !is_fullscreen) scale(2.0f);
  else SendMessage(api->main_getRootWnd()->gethWnd(),WM_KEYDOWN,keyCode,0);
  return 0;
}

int VideoWnd::onLeftButtonDown(int x, int y) {
  if (vid_clickpause) {
    if (api->core_getStatus(0) == 0) {
      api->core_userButton(0, UserButton::PLAY);
    } else
      api->core_userButton(0, UserButton::PAUSE);
    return 1;
  }
  return 0;
}

int VideoWnd::onLeftButtonDblClk(int x, int y) {
  toggleFullScreen();
  return 1;
}

int VideoWnd::onRightButtonDown(int x, int y) {
  if(!m_video_output) return 1;

  int ret;
  if(!isFullScreen()) {
    PopupMenu pm(this);
  
    PopupMenu *pms=new PopupMenu(this);
    pms->addCommand("50%",1);
    pms->addCommand("100%",2);
    pms->addCommand("150%",3);
    pms->addCommand("200%",4);
    pms->addCommand("400%",5);

    pm.addSubMenu(pms,"Scale");
    pm.addSeparator();
    pm.addCommand("Fullscreen\tAlt+Enter",6);

    ret=pm.popAtMouse();
  } else {
    PopupMenu pm(this);
    pm.addCommand("Fullscreen\tAlt+Enter",7,1);
    ret=pm.popAtMouse();
  }
  switch(ret) {
  case 1: scale(0.5f); break;
  case 2: scale(1.0f); break;
  case 3: scale(1.5f); break;
  case 4: scale(2.0f); break;
  case 5: scale(4.0f); break;
  case 6: goFullScreen(); break;
  case 7: removeFullScreen(); break;
  }
  return 1;
}

int VideoWnd::onMouseWheelUp(int click, int lines) {
#define WM_MOUSEWHEEL 0x20A
  RootWnd *p=this;
  while(p->getParent()) p=p->getParent();
  HWND h=GetParent(p->gethWnd());
  SendMessage(h,WM_MOUSEWHEEL,120<<16,0x31337);
  return 0;
}

int VideoWnd::onMouseWheelDown(int click, int lines) {
  RootWnd *p=this;
  while(p->getParent()) p=p->getParent();
  HWND h=GetParent(p->gethWnd());
  SendMessage(h,WM_MOUSEWHEEL,-120<<16,0x31337);
  return 0;
}

void VideoWnd::onEnterArea() {
  if (is_fullscreen) {
    killTimer(TIMER_HIDECURSOR);
    if (vid_hidetimer >= 0) setTimer(TIMER_HIDECURSOR, vid_hidetimer);
//    hidecursor = 0;
//    SetCursor(hidecursor ? NULL : LoadCursor(NULL, IDC_ARROW));
  }
}

void VideoWnd::onLeaveArea() {
  if (is_fullscreen) {
    killTimer(TIMER_HIDECURSOR);
  }
}

int VideoWnd::setSize(int w, int h, const char *ptype, int flipit, float aspectratio) {
  if(vid_forcevflip) flipit=!flipit;
  if(width!=w || height!=h || STRICMP(type,ptype) || !flipit!=!flip || aspect != aspectratio || (m_video_output && m_video_output->needChange())) {
    aspect=aspectratio;
    
    int dontresize=0;
    if(m_video_output) dontresize=1;

    endVideo();

    flip=!!flipit;
    width=w;
    height=h;
    type=ptype;

    if (vid_resizewnd && !dontresize) {
      RootWnd *p=getDesktopParent();
      RECT r,r2;
      getClientRect(&r);
      p->getWindowRect(&r2);
      int neww = w+((r2.right-r2.left)-(r.right-r.left));
      int newh = (int)(h*aspect)+((r2.bottom-r2.top)-(r.bottom-r.top));
      int min_w = p->getPreferences(MINIMUM_W);
      int min_h = p->getPreferences(MINIMUM_H);
      int max_w = p->getPreferences(MAXIMUM_W);
      int max_h = p->getPreferences(MAXIMUM_H);
      if (min_w != AUTOWH) neww = MAX(min_w, neww);
      if (min_h != AUTOWH) newh = MAX(min_h, newh);
      if (max_w != AUTOWH) neww = MIN(max_w, neww);
      if (max_h != AUTOWH) newh = MIN(max_h, newh);
      // this needs to be done by the main thread
      RECT res={r2.left,r2.top,neww,newh};
      api->cmd_postCommand(videowac->getGUID(),"resizewindow",(int)&res,0,(void *)p,sizeof(p),1);
      //p->resize(r2.left,r2.top,neww,newh);
    }
    
    if(!STRICMP(type,"SELF_DRAW")) return 1;

    for(int i=0;m_video_output=createVideoOutput(i);i++) {
      if(m_video_output->create(this,w,h,type,flipit,aspectratio)) {
        return 1;
      }
      delete(m_video_output);
    }
    return 0;
  }
  return 1;
}

VideoOutput *VideoWnd::createVideoOutput(int n) {
  if(!vid_overlays) n++;
  switch(n) {
  case 0: return new OverlayVideoOutput();
  case 1: return new DDrawVideoOutput();
  }
  return 0;
}

void VideoWnd::endVideo() {
  InCriticalSection cs(&videocs);
  delete(m_video_output);
  m_video_output=NULL;
  width=height=-1;
  type="";
  if(!quit) cascadeRepaint();
}

void VideoWnd::addFrame(const char *buf, int size, int time) {
  if(m_video_output) {
    InCriticalSection cs(&videocs);
    m_video_output->displayFrame(buf,size,time);
  }
}

void VideoWnd::scale(float ratio) {
  if(m_video_output && !quit) {
    RootWnd *p=getDesktopParent();
    RECT r,r2;
    getClientRect(&r);
    p->getWindowRect(&r2);
    int neww = (int)(width*ratio)+((r2.right-r2.left)-(r.right-r.left));
    int newh = (int)((int)(height*ratio)*aspect)+((r2.bottom-r2.top)-(r.bottom-r.top));
    int min_w = p->getPreferences(MINIMUM_W);
    int min_h = p->getPreferences(MINIMUM_H);
    int max_w = p->getPreferences(MAXIMUM_W);
    int max_h = p->getPreferences(MAXIMUM_H);
    if (min_w != AUTOWH) neww = MAX(min_w, neww);
    if (min_h != AUTOWH) newh = MAX(min_h, newh);
    if (max_w != AUTOWH) neww = MIN(max_w, neww);
    if (max_h != AUTOWH) newh = MIN(max_h, newh);
    p->resize(r2.left,r2.top,neww,newh);
  }
}

int VideoWnd::isFullScreen() {
  return is_fullscreen;
}

void VideoWnd::goFullScreen() {
  if(!m_video_output) return;

  HWND h=gethWnd();
  oldfsparent=GetParent(h);
  oldfsstyle=GetWindowLong(h,GWL_STYLE);
  getClientRect(&oldfsrect);
  //prev_above = GetNextWindow(h, GW_HWNDNEXT);
  prev_above = GetNextWindow(h, GW_HWNDPREV);

  //FG> be careful with client rects when using non virtual windows (derived
  // only from basewnd or using setVirtual(0)
  // in constructor) because getClientRect always report the window's drawing
  // coordinate system. In a non-virtual window
  // (like this one), the drawing coordinate system always starts at 0,0
  // (because it's an HWND), now remember that resize 
  // takes as parameters the rectangle to use within the parent, obviously you
  // don't want 0,0 there, the rectangle you 
  // want to pass to resize for a non virtual is actually
  // getClientRect+getPosition (getPosition  returns the position 
  // of the object within its parent, so a virtual window's
  // clientRect.left/top == its getPosition.x/y)

  POINT pt;
  getPosition(&pt);
  oldfsrect.left += pt.x;
  oldfsrect.top += pt.y;
  oldfsrect.right += pt.x;
  oldfsrect.bottom += pt.y;

  RECT r;
  Std::getViewport(&r,h,1);
DebugString("l %d t %d r %d b %d", r.left, r.top, r.right, r.bottom);

  SetParent(h,NULL);
  SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
  resize(&r);
  SetWindowLong(h,GWL_STYLE,WS_POPUP|WS_VISIBLE);
  is_fullscreen=1;

  m_video_output->goFullScreen();
  
  if (vid_hidetimer >= 0) setTimer(TIMER_HIDECURSOR, vid_hidetimer);	// set initial hide timer
  setFocus();
}

void VideoWnd::removeFullScreen() {
  if(!is_fullscreen) return;

  if(m_video_output) m_video_output->removeFullScreen();

  killTimer(TIMER_HIDECURSOR);

  SetWindowLong(gethWnd(),GWL_STYLE,oldfsstyle);
  SetParent(gethWnd(),oldfsparent);
  SetWindowPos(gethWnd(), prev_above, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
  resize(&oldfsrect);
  is_fullscreen=0;
}

void VideoWnd::toggleFullScreen() {
  if(is_fullscreen) removeFullScreen();
  else goFullScreen();
}

void VideoWnd::onSetVisible(int show) {
  VIDEOWND_PARENT::onSetVisible(show);
  if(!show && vid_stopvid) Video::stopPlayback();
}

int VideoWnd::onAcceleratorEvent(const char *name) {
  if(STRCASEEQL(name,"fullscreen")) {
    if(!is_fullscreen) goFullScreen();
    else {
      removeFullScreen();
      api->forwardOnKillFocus(); // resets the keyboard active keys buffer
    }
  }
  return 0;
}

int VideoWnd::kickOSD() {
  if(is_fullscreen && m_video_output) {
    if(m_video_output->showOSD()) {
      // re-set hide timer
      killTimer(TIMER_HIDECURSOR);
      if (vid_hidetimer >= 0) setTimer(TIMER_HIDECURSOR, vid_hidetimer);
      hidecursor = 0;
      SetCursor(hidecursor ? NULL : LoadCursor(NULL, IDC_ARROW));
      return 1;
    }
  }
  return 0;
}

void VideoWnd::openMiscMenu() {
  PopupMenu pop(this);
  PopupMenu *pops=new PopupMenu(this);
  pops->addCommand("50%",5);
  pops->addCommand("100%",6);
  pops->addCommand("150%",7);
  pops->addCommand("200%",8);
  pops->addCommand("400%",9);

  pop.addCommand("Fullscreen\tAlt+Enter", 1);
  pop.addSeparator();
  pop.addCommand("Half size\t1", 2);
  pop.addCommand("Normal size\t2", 3);
  pop.addCommand("Double size\t3", 4);
  pop.addSubMenu(pops,"Scale");
  pop.addSeparator();
  pop.addCommand("Video options...", 10);
  int r = pop.popAtMouse();
  switch(r) {
    case 1: goFullScreen(); break;
    case 2: scale(0.5f); break;
    case 3: scale(1.0f); break;
    case 4: scale(2.0f); break;
    case 5: scale(0.5f); break;
    case 6: scale(1.0f); break;
    case 7: scale(1.5f); break;
    case 8: scale(2.0f); break;
    case 9: scale(4.0f); break;
    case 10: openVideoOptions(); break;
  }
}

void VideoWnd::openVideoOptions() {
  svc_action *a = ActionEnum("Preferences:OPEN").getFirst();
  char strguid[256];
  nsGUID::toChar(videowac->getPrefsGUID(), strguid);
  if (a != NULL) {
    a->onAction("Preferences:OPEN", strguid);
  }
}

VideoActionsSvc::VideoActionsSvc() {
  registerAction("video:fullscreen", 10);
  registerAction("video:osd", 20);
  registerAction("video:gethwnd", 30);
  registerAction("video:scale 1x", 40);
  registerAction("video:scale 2x", 50);
  registerAction("video:tv", 60);
  registerAction("video:misc", 70);

  registerAction("vid_fs", 10); //wa5 compat
  registerAction("vid_1x", 40); //wa5 compat
  registerAction("vid_2x", 50); //wa5 compat
  registerAction("vid_tv", 60); //wa5 compat
  registerAction("vid_misc", 70); //wa5 compat

  registerAction("fullscreen", 10);	//back compat
  registerAction("video_gethwnd", 30);
  registerAction("osd", 20);
}

int VideoActionsSvc::onActionId(int actionid, const char *action, const char *param, int p1, int p2, void *data, int datalen, RootWnd *source) {
  switch (actionid) {
    case 10:
      // FUCKO: try to find window by target (if there is one)
      if (videownd != NULL) {
        videownd->toggleFullScreen();
      }
    return 1;
    case 20:
      // FUCKO: try to find window by target (if there is one)
      if (videownd != NULL) {
        videownd->kickOSD();
      }
    return 1;
    case 30:
      // FUCKO: try to find window by target (if there is one)
      if(videownd) return (int)videownd->gethWnd();
      else return 0;
    case 40:
      // FUCKO: try to find window by target (if there is one)
      if (videownd != NULL) {
        videownd->scale(1.0f);
      }
    return 1;
    case 50:
      // FUCKO: try to find window by target (if there is one)
      if (videownd != NULL) {
        videownd->scale(2.0f);
      }
    return 1;
    case 60:
      // FUCKO: try to find window by target (if there is one)
      if (videownd != NULL) {
        static GUID g={ 0x6b0edf80, 0xc9a5, 0x11d3, { 0x9f, 0x26, 0x00, 0xc0, 0x4f, 0x39, 0xff, 0xc6 } };   //guid:ml
        WASABI_API_WNDMGR->skinwnd_toggleByGuid(g);
      }
    return 1;
    case 70:
      // FUCKO: try to find window by target (if there is one)
      if (videownd != NULL) {
        videownd->openMiscMenu();
      }
    return 1;
  }
  return 0;
}
