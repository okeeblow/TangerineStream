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
#include "xuiwa2slider.h"
#include "../bfc/bitmap.h"
#include "../bfc/canvas.h"
#include "../studio/api.h"

char Wa2SliderXuiObjectStr[] = "Wa2Slider"; // This is the xml tag
char Wa2SliderXuiSvcName[] = "Wa2Slider XuiObject Service";

Wa2Slider::Wa2Slider() {
  imagesBitmap=0;
  spacing=0;
  setThumbCentered(0);
  setThumbOffset(1);
  action=ACT_NONE;
  
  xuihandle=newXuiHandle();
  addParam(xuihandle, "images", WA2SLIDER_IMAGES, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "imagesspacing", WA2SLIDER_IMAGESSPACING, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "thumb", WA2SLIDER_THUMB, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "downThumb", WA2SLIDER_DOWNTHUMB, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "action", WA2SLIDER_ACTION, XUI_ATTRIBUTE_IMPLIED);
}

Wa2Slider::~Wa2Slider() {
  api->core_delCallback(0, this);
  delete(imagesBitmap);
}

int Wa2Slider::setXuiParam(int _xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle == _xuihandle) {
    switch(xmlattributeid) {
      case WA2SLIDER_IMAGES: images=value; return 1;
      case WA2SLIDER_IMAGESSPACING: spacing=ATOI(value); return 1;
      case WA2SLIDER_THUMB: setThumbBmp(value); return 1;
      case WA2SLIDER_DOWNTHUMB: setThumbDownBmp(value); return 1;
      case WA2SLIDER_ACTION:
        if(STRCASEEQL(value,"volume")) action=ACT_VOLUME;
        if(STRCASEEQL(value,"balance")) action=ACT_BALANCE;
        return 1;
    }
  }
  return WA2SLIDER_PARENT::setXuiParam(_xuihandle,xmlattributeid,xmlattributename,value);
}

int Wa2Slider::onInit() {
  setNoDefaultBackground(1);
  WA2SLIDER_PARENT::onInit();

  imagesBitmap=new SkinBitmap(images);

  if(action==ACT_VOLUME) corecb_onVolumeChange(api->core_getVolume(0));
  if(action==ACT_BALANCE) {
    corecb_onPanChange(api->core_getPan(0));
    setHotPosition(FULL/2);
  }
  
  api->core_addCallback(0, this);

  return 0;
}

int Wa2Slider::onPaint(Canvas *canvas) {
  if(imagesBitmap->getHeight() && spacing) {
    RECT r,r2;
    getClientRect(&r2);
    int nb=(imagesBitmap->getHeight()/spacing)-1;
    int which;
    if(action==ACT_VOLUME) which=getSliderPosition()*nb/FULL;
    if(action==ACT_BALANCE) {
      int p=getSliderPosition();
      int f=FULL/2;
      if(p>f) {
        which=(getSliderPosition()-f)*nb/f;
      } else if(p<f) {
        which=(f-getSliderPosition())*nb/f;
      } else which=0;
    }
    r.left=0;
    r.top=which*spacing;
    r.bottom=which*spacing+(r2.bottom-r2.top);
    r.right=r2.right-r2.left;
    imagesBitmap->blitToRect(canvas,&r,&r2);
  }
  return WA2SLIDER_PARENT::onPaint(canvas);
}

int Wa2Slider::onSetPosition() {
  WA2SLIDER_PARENT::onSetPosition();
  int pos = getSliderPosition();	// get slider pos
  int realpos = (int)(((double)pos * 255.f) / 65535.f);
  if(action==ACT_VOLUME) {
    api->core_setVolume(0,realpos);
    api->core_setCustomMsg(0, StringPrintf("Volume: %i%%",pos*100/65535));
  }
  if(action==ACT_BALANCE) {
    int p=realpos-127;
    api->core_setPan(0,p);
    String b;
    if(!p) b="Center";
    else if(p>0) b.printf("%i%% Right",p*100/127);
    else b.printf("%i%% Left",(-p)*100/127);
    api->core_setCustomMsg(0, StringPrintf("Balance: %s",b.getValue()));
  }
  return 0;
}

int Wa2Slider::corecb_onVolumeChange(int newvol) {
  if (action!=ACT_VOLUME) return 0;
  if (getSeekStatus()) return 0;
  int realpos = (int)(((double)newvol * 65535.f) / 255.f);
  setPosition(realpos);
  return 0;
}

int Wa2Slider::corecb_onPanChange(int newpan) {
  if (action!=ACT_BALANCE) return 0;
  if (getSeekStatus()) return 0;
  int realpos = (int)(((double)(newpan+127) * 65535.f) / 255.f);
  setPosition(realpos);
  return 0;
}
