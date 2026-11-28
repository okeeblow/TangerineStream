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
#ifndef __XUIBROWSER_H
#define __XUIBROWSER_H

#include "../common/minibrowserwnd.h"
#include "../common/script/objcontroller.h"
#include "../studio/api.h"

#define MAIN_BROWSER_ATOM_NAME "browser.main.object"
#define SCRIPTBROWSERWND_PARENT MiniBrowserWnd 

// -----------------------------------------------------------------------------------------------------
class ScriptBrowserWnd : public SCRIPTBROWSERWND_PARENT {
  
  public:

    ScriptBrowserWnd();
    virtual ~ScriptBrowserWnd();

    virtual void onSetVisible(int v);

    // XuiObject

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    // MiniBrowserWnd
    int onBeforeNavigate(const char *url, int flags, const char *frame);
    void onDocumentComplete(const char *url);

    virtual int navigateUrl(const char *url);

    // --

    void setUrl(const char *url);
    void setMainMB(int tf);

  private:

    enum {
      BROWSER_SETURL = 0,
      BROWSER_SETMAINMB,
      BROWSER_SETTARGETNAME,
      BROWSER_SETSCROLLBARS,
    };

    int translateScrollbarFlag(const char *scrollbarflag);

    String defurl;
    int myxuihandle;
};

// -----------------------------------------------------------------------------------------------------
class BrowserScriptController : public ScriptObjectControllerI {
  public:

    virtual const char *getClassName() { return "Browser"; }
    virtual const char *getAncestorClassName() { return "GuiObject"; }
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid() { return browserGuid; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  public:
    static scriptVar browser_navigateUrl(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar url);
    static scriptVar browser_back(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar browser_forward(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar browser_home(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar browser_stop(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar browser_refresh(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar browser_setTargetName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar name);
    static scriptVar browser_onDocumentComplete(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar url);
    static scriptVar browser_onBeforeNavigate(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar url, scriptVar flags, scriptVar framename);

  private:

    static function_descriptor_struct exportedFunction[];
};

extern BrowserScriptController *browserController;

extern char browserXuiObjectStr[];
extern char browserXuiSvcName[];
class BrowserXuiSvc : public XuiObjectSvc<ScriptBrowserWnd, browserXuiObjectStr, browserXuiSvcName> {};

#endif
