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
#ifndef __GRID_H
#define __GRID_H

#include "../common/guiobjwnd.h"
#include "../bfc/autobitmap.h"
#include "bfc/region.h"

#define GRID_PARENT GuiObjectWnd 

// -----------------------------------------------------------------------
class Grid : public GRID_PARENT {
  
  public:

    Grid();
    virtual ~Grid();

    virtual int onPaint(Canvas *c); 
    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setGridImage(const char *elementname, int what);
    virtual void getGridRect(RECT *r) { getClientRect(r); }

    virtual Region *getRegion() { return &reg; }
    virtual int onInit();
    virtual int onResize();

  private:
    void doPaint(Canvas *canvas, int dorgn=0);

    enum {
      GRID_SETTOPLEFT= 0,
      GRID_SETTOP,
      GRID_SETTOPRIGHT,
      GRID_SETLEFT,
      GRID_SETMIDDLE,
      GRID_SETRIGHT,
      GRID_SETBOTTOMLEFT,
      GRID_SETBOTTOM,
      GRID_SETBOTTOMRIGHT,
    };

    int myxuihandle;

    AutoSkinBitmap topleft,    top,    topright;
    AutoSkinBitmap left,       middle, right;
    AutoSkinBitmap bottomleft, bottom, bottomright;
    RegionI reg;
};


// -----------------------------------------------------------------------
extern char GridXuiObjectStr[];
extern char GridXuiSvcName[];
class GridXuiSvc : public XuiObjectSvc<Grid, GridXuiObjectStr, GridXuiSvcName> {};

#endif
