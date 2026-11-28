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
#include "xuiwa2time.h"
#include "../bfc/bitmap.h"
#include "../bfc/canvas.h"
#include "../studio/api.h"

char Wa2TimeXuiObjectStr[] = "Wa2Time"; // This is the xml tag
char Wa2TimeXuiSvcName[] = "Wa2Time XuiObject Service";

Wa2Time::Wa2Time() {
  isplaying=0;
  elapsed=WASABI_API_CONFIG->getIntPrivate("timer_elapsed",0);
  el=0;
  xuihandle = newXuiHandle();
  addParam(xuihandle, "numbers", WA2TIME_NUMBERS, XUI_ATTRIBUTE_REQUIRED);
}

Wa2Time::~Wa2Time() {
  api->core_delCallback(0, this);
}

int Wa2Time::setXuiParam(int _xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle == _xuihandle) {
    switch (xmlattributeid) {
      case WA2TIME_NUMBERS:
        numbers=value;
        return 1;
    }
  }
  return WA2TIME_PARENT::setXuiParam(_xuihandle,xmlattributeid,xmlattributename,value);
}

int Wa2Time::onInit() {
  WA2TIME_PARENT::onInit();

  api->core_addCallback(0, this);
  setTimer(1,500);
  isplaying=api->core_getStatus(0)!=0;
  return 0;
}

int Wa2Time::onPaint(Canvas *canvas) {
  if(!isplaying) return 0;
  if(mins.isempty() || secs.isempty()) return 0;

  RECT r,r2;
  r.top=0;
  getClientRect(&r2);

  int i;
  for(i=0;i<3;i++) {
    char p=mins.getValue()[i];
    if(p==' ') p=10;
    else p-='0';
    int off=(int)p*9;
    r.left=off;
    r.right=off+9;
    r.bottom=numbers.getHeight();
    numbers.getBitmap()->blitToRect(canvas,&r,&r2);
    r2.left+=12;
    r2.right+=12;
  }

  r2.left+=6;
  r2.right+=6;

  for(i=0;i<2;i++) {
    char p=secs.getValue()[i];
    p-='0';
    int off=(int)p*9;
    r.left=off;
    r.right=off+9;
    r.bottom=numbers.getHeight();
    numbers.getBitmap()->blitToRect(canvas,&r,&r2);
    r2.left+=12;
    r2.right+=12;
  }

  if(el) {
    getClientRect(&r2);
    r2.right=r2.left+9;
    if(numbers.getWidth()==108) {
      r.left=99;
      r.right=99+9;
    } else {
      // take the bar from the number 2
      r.left=9*2;
      r.right=9*3;
      r.top=6;
      r.bottom=7;
      r2.top+=6;
      r2.bottom=r2.top+1;
    }
    numbers.getBitmap()->blitToRect(canvas,&r,&r2);
  }

  return 0;
}

int Wa2Time::corecb_onStarted() {
  isplaying=1;
  timerCallback(1);
  return 0; 
}

int Wa2Time::corecb_onStopped() {
  isplaying=0;
  invalidate();
  return 0;
}

void Wa2Time::timerCallback(int id) {
  if(id==1 && isplaying) {
    int cp=api->core_getPosition(0);
    if(cp<0) {
      mins=" 00";
      secs="00";
      invalidate();
      return;
    }
    int p;
    int len = api->core_getLength(0);
    el = elapsed;
    if (len==-1) el=0; // no elapsed time on http streams, etc...
    if (!el) p=cp/1000;
    else p=(len-cp)/1000;
    int m=p/60;
    if(m<100) mins.printf(" %02d",p/60);
    else mins.printf("%03d",p/60);
    secs.printf("%02d",p%60);
    invalidate();
  }
}

int Wa2Time::onLeftButtonDown(int x, int y) {
  elapsed=!elapsed;
  WASABI_API_CONFIG->setIntPrivate("timer_elapsed", elapsed);
  timerCallback(1);
  return 0;
}
