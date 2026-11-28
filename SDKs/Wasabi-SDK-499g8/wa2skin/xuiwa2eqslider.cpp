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
#include "xuiwa2eqslider.h"
#include "../bfc/bitmap.h"
#include "../bfc/canvas.h"
#include "../studio/api.h"

char Wa2EqSliderXuiObjectStr[] = "Wa2EqSlider"; // This is the xml tag
char Wa2EqSliderXuiSvcName[] = "Wa2EqSlider XuiObject Service";

#define NOTIFYMSG_EQ_HELLO 0x3003

Wa2EqSlider::Wa2EqSlider() {
  imagesBitmap=0;
  band=0;
  isactive=0;
  setThumbCentered(0);
  setThumbOffset(1);
  xuihandle = newXuiHandle();  
  addParam(xuihandle, "band", WA2EQSLIDER_BAND, XUI_ATTRIBUTE_IMPLIED);
}

Wa2EqSlider::~Wa2EqSlider() {
  api->core_delCallback(0, this);
  delete(imagesBitmap);
}

int Wa2EqSlider::setXuiParam(int _xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle == _xuihandle) {
    switch (xmlattributeid) {
      case WA2EQSLIDER_BAND:
        band=ATOI(value);
        return 1;
    }
  }
  return WA2EQSLIDER_PARENT::setXuiParam(_xuihandle,xmlattributeid,xmlattributename,value);
}

int Wa2EqSlider::onInit() {
  setNoDefaultBackground(1);
  WA2EQSLIDER_PARENT::onInit();

  imagesBitmap=new SkinBitmap("player.wa2.eq.slider");

  api->core_addCallback(0, this);

  setThumbBmp("player.eq.button.bandslider");
  setThumbDownBmp("player.eq.button.bandslider.pressed");
  setOrientation(1);
  setHotPosition(32768);

  if(!band) corecb_onEQPreampChange(api->core_getEqPreamp(0));
  else corecb_onEQBandChange(band-1,api->core_getEqBand(0,band-1));
  return 0;
}

int Wa2EqSlider::onPaint(Canvas *canvas) {
  if(imagesBitmap->getHeight()) {
    RECT r,r2;
    getClientRect(&r2);
    int p=getSliderPosition();
    int which=0;
    if(p<FULL/2) {
      which=p*14/(FULL/2);
      r.left=which*15;
      r.right=r.left+14;
      r.top=0;
      r.bottom=63;
    } else {
      which=(p-FULL/2)*13/(FULL/2);
      r.left=which*15;
      r.right=r.left+14;
      r.top=65;
      r.bottom=65+63;
    }
    imagesBitmap->blitToRect(canvas,&r,&r2);
  }
  return WA2EQSLIDER_PARENT::onPaint(canvas);
}

int Wa2EqSlider::onSetPosition() {
  WA2EQSLIDER_PARENT::onSetPosition();
  int pos = getSliderPosition();	// get slider pos
  pos-=32767;
  int realpos = (int)(((double)pos * 127.f) / 32767.f);
  if(!band) api->core_setEqPreamp(0,realpos);
  else api->core_setEqBand(0,band-1,realpos);
  return 0;
}

int Wa2EqSlider::corecb_onEQBandChange(int pband, int newval) {
  if(pband!=(band-1)) return 0;
  int realpos = (int)(((double)newval * 32767.f) / 127.f);
  realpos+=32767;
  setPosition(realpos);
  return 0;
}

int Wa2EqSlider::corecb_onEQPreampChange(int newval) {
  if(band!=0) return 0;
  int realpos = (int)(((double)newval * 32767.f) / 127.f);
  realpos+=32767;
  setPosition(realpos);
  return 0;
}

int Wa2EqSlider::onLeftButtonDown(int x, int y) { 
  isactive=1; 
  return WA2EQSLIDER_PARENT::onLeftButtonDown(x,y); 
}

int Wa2EqSlider::onMouseMove(int x, int y) {
  if(isactive && band!=0) {
    RootWnd *parent=getParent();
    if(parent) {
      RootWnd *wnd=parent->findRootWndChild(x,y);
      if(wnd && wnd!=this) {    
        if(wnd->childNotify(this,NOTIFYMSG_EQ_HELLO,0,0)) { // will return 1 if it's another EQBand
          onLeftButtonUp(x,y);
          wnd->onLeftButtonDown(x,y);
        }
      }
    }
  }
  return WA2EQSLIDER_PARENT::onMouseMove(x,y);
}

int Wa2EqSlider::onLeftButtonUp(int x, int y) { 
  isactive=0; 
  return WA2EQSLIDER_PARENT::onLeftButtonUp(x,y); 
}

int Wa2EqSlider::childNotify(RootWnd *child, int msg, int param1, int param2) {
  if(msg==NOTIFYMSG_EQ_HELLO && band!=0) return 1;
  return WA2EQSLIDER_PARENT::childNotify(child,msg,param1,param2);
}
