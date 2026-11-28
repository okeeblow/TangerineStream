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
#ifndef __SCRIPTTABSHEET_H
#define __SCRIPTTABSHEET_H

#include "../bfc/wnds/typesheet.h"
#include "../common/script/objcontroller.h"

#define  SCRIPTTABSHEET_PARENT TypeSheet

// -----------------------------------------------------------------------
// Your wnd object class

class ScriptTabSheet : public  SCRIPTTABSHEET_PARENT {
  
  public:

    ScriptTabSheet();
    virtual ~ScriptTabSheet();

    // XuiObject automatically calls this back for all parameters registered using addParam
    // encountered in the xml source
    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setWindowType(const char *elementname);
    void setChildrenIds(const char *paramvalue);
    void setType(const char *element);
    virtual int onInit();

    void setContentMarginX(const char *value, int what);

  private:

    // a list of IDs for our xml attributes, we use them in addParam() in the constructor
    enum {
       SCRIPTTABSHEET_SETWINDOWTYPE = 0,
       SCRIPTTABSHEET_SETCHILDREN,
       SCRIPTTABSHEET_SETTYPE,
       SCRIPTTABSHEET_SETCONTENTMARGINLEFT,
       SCRIPTTABSHEET_SETCONTENTMARGINTOP,
       SCRIPTTABSHEET_SETCONTENTMARGINRIGHT,
       SCRIPTTABSHEET_SETCONTENTMARGINBOTTOM,
    };
    int myxuihandle;
    PtrList<String> children_id;
    void reloadChildren();
    int type;
    String wndtype;

    int left_margin, right_margin, top_margin, bottom_margin;
};


// -----------------------------------------------------------------------
// This defines the svc_xuiObject that exposes your wnd object

extern char ScriptTabSheetXuiObjectStr[];
extern char ScriptTabSheetXuiSvcName[];
class ScriptTabSheetXuiSvc : public XuiObjectSvc<ScriptTabSheet, ScriptTabSheetXuiObjectStr, ScriptTabSheetXuiSvcName> {};

// -----------------------------------------------------------------------------------------------------
class ScriptTabSheetController : public ScriptObjectControllerI {
  public:

    virtual const char *getClassName() { return "TabSheet"; }
    virtual const char *getAncestorClassName() { return "GuiObject"; }
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid() { return tabsheetGuid; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  public:
    static scriptVar tabsheet_getCurPage(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar tabsheet_setCurPage(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a);
    static scriptVar tabsheet_getNumPages(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar tabsheet_nextPage(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar tabsheet_previousPage(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

  private:

    static function_descriptor_struct exportedFunction[];
};

extern ScriptTabSheetController *tabsheetController;

#endif

