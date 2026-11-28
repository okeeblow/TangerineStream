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
#ifndef __XUIFRAME_H
#define __XUIFRAME_H

#include "../common/framewnd.h"
#include "../bfc/autobitmap.h"

#define SCRIPTFRAME_PARENT FrameWnd 

// -----------------------------------------------------------------------
// Your wnd object class

class ScriptFrame : public SCRIPTFRAME_PARENT {
  
  public:

    ScriptFrame();
    virtual ~ScriptFrame();

    virtual int onInit();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setOrientation(const char *elementname);
    void setLeft(const char *groupname);
    void setRight(const char *groupname);
    void setFrom(const char *from);
    void setWidth(const char *w);
    void setResize(const char *r);

    virtual int wantRenderBaseTexture() { return 0; }

    void onResizeChildren(RECT leftr, RECT rightr);

  private:

    // a list of IDs for our xml attributes, we use them in addParam() in the constructor
    enum {
      SCRIPTFRAME_SETORIENTATION = 0,
      SCRIPTFRAME_SETLEFT,
      SCRIPTFRAME_SETRIGHT,
      SCRIPTFRAME_SETFROM,
      SCRIPTFRAME_SETWIDTH,
      SCRIPTFRAME_SETRESIZEABLE,
    };

    int myxuihandle;
    String left, right;
    RootWnd *rootwndleft, *rootwndright;
    int from, orientation, resizable;
    int width;
};


// -----------------------------------------------------------------------
// This defines the svc_xuiObject that exposes your wnd object

extern char ScriptFrameXuiObjectStr[];
extern char ScriptFrameXuiSvcName[];
class FrameXuiSvc : public XuiObjectSvc<ScriptFrame, ScriptFrameXuiObjectStr, ScriptFrameXuiSvcName> {};

#endif
