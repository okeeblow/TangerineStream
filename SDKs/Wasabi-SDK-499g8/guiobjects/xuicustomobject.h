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
#ifndef __CustomObject_H
#define __CustomObject_H

#include "../common/guiobjwnd.h"
#include "../bfc/autobitmap.h"
#include "../bfc/wnds/customobject.h"

#define  CUSTOMOBJECT_PARENT GuiObjectWnd

// -----------------------------------------------------------------------
class XuiCustomObject : public  CUSTOMOBJECT_PARENT, public CustomObjectI {
  
  public:
                             
    XuiCustomObject();
    virtual ~XuiCustomObject();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setGroup(const char *elementname);
    virtual void customobject_setRootWnd(RootWnd *w);
    virtual RootWnd *customobject_getRootWnd();

  private:

    enum {
       CUSTOMOBJECT_SETGROUP        = 0,
    };
    int myxuihandle;
    String groupid;
};


// -----------------------------------------------------------------------

extern char CustomObjectXuiObjectStr[];
extern char CustomObjectXuiSvcName[];
class CustomObjectXuiSvc : public XuiObjectSvc<XuiCustomObject, CustomObjectXuiObjectStr, CustomObjectXuiSvcName> {};

#endif
