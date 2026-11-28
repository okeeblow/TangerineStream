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
#ifndef __SKINSHOT_H
#define __SKINSHOT_H

#include "../common/guiobjwnd.h"
#include "../bfc/bitmap.h"
#include "../bfc/wnds/scbkgwnd.h"

class SkinItem;

// {2914EC8D-FD18-4620-83EF-B6D720C9DC8D}
static const GUID skinshotGuid = 
{ 0x2914ec8d, 0xfd18, 0x4620, { 0x83, 0xef, 0xb6, 0xd7, 0x20, 0xc9, 0xdc, 0x8d } };

#define SKINSHOT_PARENT GuiObjectWnd

// -----------------------------------------------------------------------
class XuiSkinShot : public SKINSHOT_PARENT {
  
  public:

    XuiSkinShot();
    virtual ~XuiSkinShot();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void set(const char *elementname);

    virtual int onPaint(Canvas *canvas);

    void setSkinItem(SkinItem *item);
 
  private:

    enum {
      SKINSHOT_SET = 0,
    };

    int myxuihandle;
	  SkinBitmap *bitmap;
};

// -----------------------------------------------------------------------
extern char SkinShotXuiObjectStr[];
extern char SkinShotXuiSvcName[];
class SkinShotXuiSvc : public XuiObjectSvc<XuiSkinShot, SkinShotXuiObjectStr, SkinShotXuiSvcName> {};

#endif
