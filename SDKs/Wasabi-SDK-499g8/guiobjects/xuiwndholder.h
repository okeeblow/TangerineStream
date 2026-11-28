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
#ifndef __XUIWNDHOLDER_H
#define __XUIWNDHOLDER_H

#include "../studio/api.h"
#include "../bfc/wnds/wndholder.h"
#include "../bfc/autobitmap.h"
#include "../common/script/scriptguid.h"
#include "../common/script/objcontroller.h"

#define XUIWNDHOLDER_PARENT WindowHolderWnd 

// -----------------------------------------------------------------------
class XuiWindowHolder : public XUIWNDHOLDER_PARENT {
  
  public:

    XuiWindowHolder();
    virtual ~XuiWindowHolder();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setRegionFromMap(ScriptObject *map, int byte, int inverse);
    void setRegion(ScriptObject *region);
    GUID *parseGUID(const char *id);

  private:

    enum {
      XUIWNDHOLDER_ADDHOLD = 10,
      XUIWNDHOLDER_SETNOSHOWCMDBAR = 20,
      XUIWNDHOLDER_SETNOANIMRECTS = 30,
      XUIWNDHOLDER_SETAUTOOPEN = 40,
      XUIWNDHOLDER_SETAUTOCLOSE = 50,
      XUIWNDHOLDER_SETAUTOFOCUS = 60,
      XUIWNDHOLDER_SETAUTOAVAILABLE = 70,
    };
    int myxuihandle;
};

// -----------------------------------------------------------------------------------------------------
class WindowHolderScriptController: public ScriptObjectControllerI {
  public:

    virtual const char *getClassName() { return "WindowHolder"; }
    virtual const char *getAncestorClassName() { return "GuiObject"; }
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid() { return windowHolderGuid; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  public:
    static scriptVar script_getGUID(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar url);
    static scriptVar script_setRegionFromMap(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar map, scriptVar byte, scriptVar inv);
    static scriptVar script_setRegion(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar reg);
    static scriptVar script_getContent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

  private:

    static function_descriptor_struct exportedFunction[];
};

extern WindowHolderScriptController *windowHolderController;


// -----------------------------------------------------------------------
extern char WindowHolderXuiObjectStr[];
extern char WindowHolderXuiObjectStrCompat[];
extern char WindowHolderXuiSvcName[];
class WindowHolderXuiSvc : public XuiObjectSvc<XuiWindowHolder, WindowHolderXuiObjectStr, WindowHolderXuiSvcName> {};
class WindowHolderXuiSvc2 : public XuiObjectSvc<XuiWindowHolder, WindowHolderXuiObjectStrCompat, WindowHolderXuiSvcName> {};

#endif
