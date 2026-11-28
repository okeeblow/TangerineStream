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
#ifndef _XUIWA2SLIDER_H
#define _XUIWA2SLIDER_H

#include <common/guiobjwnd.h>
#include <common/slider.h>
#include <core/corecb.h>

#define WA2SLIDER_PARENT SliderWnd

class Wa2Slider : public WA2SLIDER_PARENT, public CoreCallbackI {
  public:
    Wa2Slider();
    virtual ~Wa2Slider();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual int onInit();
    virtual int onPaint(Canvas *canvas);

    virtual int onSetPosition();		// called constantly as mouse moves

    virtual int corecb_onVolumeChange(int newvol);
    virtual int corecb_onPanChange(int newpan);

  private:
    String images;
    SkinBitmap *imagesBitmap;
    int spacing;
    int action;
    int xuihandle;

    enum {
      ACT_NONE=0,
      ACT_VOLUME,
      ACT_BALANCE,
    };
    
    enum {
      WA2SLIDER_IMAGES,
      WA2SLIDER_IMAGESSPACING,
      WA2SLIDER_THUMB,
      WA2SLIDER_DOWNTHUMB,
      WA2SLIDER_ACTION,
    };
};

// -----------------------------------------------------------------------
extern char Wa2SliderXuiObjectStr[];
extern char Wa2SliderXuiSvcName[];
class Wa2SliderXuiSvc : public XuiObjectSvc<Wa2Slider, Wa2SliderXuiObjectStr, Wa2SliderXuiSvcName> {};

#endif
