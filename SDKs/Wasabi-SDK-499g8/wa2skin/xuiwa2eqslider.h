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
#ifndef _XUIWA2EQSLIDER_H
#define _XUIWA2EQSLIDER_H

#include <common/guiobjwnd.h>
#include <common/slider.h>
#include <core/corecb.h>

#define WA2EQSLIDER_PARENT SliderWnd

class Wa2EqSlider : public WA2EQSLIDER_PARENT, public CoreCallbackI {
  public:
    Wa2EqSlider();
    virtual ~Wa2EqSlider();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual int onInit();
    virtual int onPaint(Canvas *canvas);

    virtual int onSetPosition();

    virtual int corecb_onEQBandChange(int band, int newval);
    virtual int corecb_onEQPreampChange(int newval);

    virtual int onLeftButtonDown(int x, int y);
    virtual int onMouseMove(int x, int y);	// only called when mouse captured
    virtual int onLeftButtonUp(int x, int y);
    virtual int childNotify(RootWnd *child, int msg, int param1=0, int param2=0);

  private:
    String images;
    SkinBitmap *imagesBitmap;
    int band, isactive;
    int xuihandle;
    
    enum {
      WA2EQSLIDER_BAND,
    };
};

// -----------------------------------------------------------------------
extern char Wa2EqSliderXuiObjectStr[];
extern char Wa2EqSliderXuiSvcName[];
class Wa2EqSliderXuiSvc : public XuiObjectSvc<Wa2EqSlider, Wa2EqSliderXuiObjectStr, Wa2EqSliderXuiSvcName> {};

#endif
