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
#ifndef __SCRIPTTITLEBOX_H
#define __SCRIPTTITLEBOX_H

#include "../bfc/wnds/titlebox.h"
#include "../common/script/c_script/h_guiobject.h"
#include "../bfc/accessible.h"

#define  SCRIPTTITLEBOX_PARENT TitleBox

// -----------------------------------------------------------------------
// Your wnd object class
class ScriptTitleBox : public SCRIPTTITLEBOX_PARENT {
  
  public:

    ScriptTitleBox();
    virtual ~ScriptTitleBox();

    // XuiObject automatically calls this back for all parameters registered using addParam
    // encountered in the xml source
    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual int accessibility_getRole() { return ROLE_SYSTEM_GROUPING; }

  private:

    // a list of IDs for our xml attributes, we use them in addParam() in the constructor
    enum {
       SCRIPTTITLEBOX_TITLE= 0,
       SCRIPTTITLEBOX_CONTENT,
       SCRIPTTITLEBOX_CENTERED,
       SCRIPTTITLEBOX_SUFFIX,
    };
    int myxuihandle;
};

// -----------------------------------------------------------------------
// This defines the svc_xuiObject that exposes your wnd object

extern char ScriptTitleBoxXuiObjectStr[];
extern char ScriptTitleBoxXuiSvcName[];
class ScriptTitleBoxXuiSvc : public XuiObjectSvc<ScriptTitleBox, ScriptTitleBoxXuiObjectStr, ScriptTitleBoxXuiSvcName> {};

#endif
