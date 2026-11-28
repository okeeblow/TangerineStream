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
#ifndef _XUIWA2MS_H
#define _XUIWA2MS_H

#include <common/guiobjwnd.h>
#include <bfc/basewnd.h>
#include <core/corecb.h>

#define WA2MS_PARENT GuiObjectWnd

class Wa2Ms : public WA2MS_PARENT, public CoreCallbackI {
  public:
    Wa2Ms();
    virtual ~Wa2Ms();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual int onInit();
    virtual int onPaint(Canvas *canvas);

    virtual int corecb_onInfoChange(const char *info);
    virtual int corecb_onStopped();

  private:
    SkinBitmap *inactiveImage,*activeImage;
    String inactivename,activename,str;
    int isactive;
    int xuihandle;
    
    enum {
      WA2MS_INACTIVE,
      WA2MS_ACTIVE,
      WA2MS_STRING,
    };
};

// -----------------------------------------------------------------------
extern char Wa2MsXuiObjectStr[];
extern char Wa2MsXuiSvcName[];
class Wa2MsXuiSvc : public XuiObjectSvc<Wa2Ms, Wa2MsXuiObjectStr, Wa2MsXuiSvcName> {};

#endif
