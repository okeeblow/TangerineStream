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
#ifndef __PROGRESSGRID_H
#define __PROGRESSGRID_H

#include <common/guiobjwnd.h>
#include "xuigrid.h"
#include <bfc/autobitmap.h>
#include <core/corecb.h>

#define PROGRESSGRID_PARENT Grid

enum {
  PROGRESSGRID_TOP = 0,
  PROGRESSGRID_LEFT = 1,
  PROGRESSGRID_RIGHT = 2,
  PROGRESSGRID_BOTTOM = 3,
};

// -----------------------------------------------------------------------
class ProgressGrid : public PROGRESSGRID_PARENT, public CoreCallbackI {
  
  public:

    ProgressGrid();
    virtual ~ProgressGrid();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual void setOrientation(const char *or);
    virtual void getGridRect(RECT *r);

    virtual void setProgress(float p); // 0..1

    virtual int onInit();
    virtual int corecb_onSeeked(int newpos);
    virtual int corecb_onStarted();
    virtual int corecb_onStopped();
    virtual void timerCallback(int id);

  private:

    enum {
      PROGRESSGRID_SETORIENTATION = 0,
    };

    int orientation;
    int myxuihandle;
    float progress;
    int started;
};


// -----------------------------------------------------------------------
extern char ProgressGridXuiObjectStr[];
extern char ProgressGridXuiSvcName[];
class ProgressGridXuiSvc : public XuiObjectSvc<ProgressGrid, ProgressGridXuiObjectStr, ProgressGridXuiSvcName> {};

#endif
