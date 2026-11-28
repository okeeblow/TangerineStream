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
#ifndef __XuiTemplateList_H
#define __XuiTemplateList_H

#include "../../common/guiobjwnd.h"
#include "../../common/autobitmap.h"

#define  XuiTemplateList_PARENT GuiObjectWnd 

// -----------------------------------------------------------------------
// Your wnd object class

class XuiTemplateList : public  XuiTemplateList_PARENT {
  
  public:

    XuiTemplateList();
    virtual ~XuiTemplateList();

    // BaseWnd

    virtual int onPaint(Canvas *c); // this is for painting our object
    virtual int getPreferences(int what); // we override this to return default w/h if none is provided

    // XuiObject automatically calls this back for all parameters registered using addParam
    // encountered in the xml source
    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    // those are called by yourself in setXuiParam
    void setImage(const char *elementname);
//    void setSomeMoreOption(const char *paramvalue);
//    void setSomeMoreOption(const char *paramvalue);

  private:

    // the image we use to draw ourselves
    AutoSkinBitmap image;

    // a list of IDs for our xml attributes, we use them in addParam() in the constructor
    enum {
       XuiTemplateList_SETIMAGE        = 0,
  //     XuiTemplateList_SOMEMOREOPTION,
  //     XuiTemplateList_SOMEMOREOPTION,
    };
    int myxuihandle;
};


// -----------------------------------------------------------------------
// This defines the svc_xuiObject that exposes your wnd object

extern char XuiTemplateListXuiObjectStr[];
extern char XuiTemplateListXuiSvcName[];
class XuiTemplateListXuiSvc : public XuiObjectSvc<XuiTemplateList, XuiTemplateListXuiObjectStr, XuiTemplateListXuiSvcName> {};

#endif
