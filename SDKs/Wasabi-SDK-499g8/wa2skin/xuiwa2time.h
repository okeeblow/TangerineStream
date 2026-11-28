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
#ifndef _XUIWA2TIME_H
#define _XUIWA2TIME_H

#include <common/guiobjwnd.h>
#include <bfc/basewnd.h>
#include <core/corecb.h>

#define WA2TIME_PARENT GuiObjectWnd

class Wa2Time : public WA2TIME_PARENT, public CoreCallbackI {
  public:
    Wa2Time();
    virtual ~Wa2Time();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual int onInit();
    virtual int onPaint(Canvas *canvas);

    virtual int corecb_onStarted();
    virtual int corecb_onStopped();
    virtual int corecb_onPaused() { return 0; }
    virtual int corecb_onUnpaused() { return 0; }
    virtual int corecb_onSeeked(int newpos) { timerCallback(1); return 0; }

    virtual void timerCallback(int id);

    virtual int onLeftButtonDown(int x, int y); 

  private:
    AutoSkinBitmap numbers;
    String numbersname;
    String mins,secs;
    int isplaying;
    int elapsed,el;
    int xuihandle;

    enum {
      WA2TIME_NUMBERS,
    };
};

// -----------------------------------------------------------------------
extern char Wa2TimeXuiObjectStr[];
extern char Wa2TimeXuiSvcName[];
class Wa2TimeXuiSvc : public XuiObjectSvc<Wa2Time, Wa2TimeXuiObjectStr, Wa2TimeXuiSvcName> {};

#endif
