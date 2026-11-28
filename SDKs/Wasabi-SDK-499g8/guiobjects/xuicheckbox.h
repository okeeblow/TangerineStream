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
#ifndef __SCRIPTCHECKBOX_H
#define __SCRIPTCHECKBOX_H

#include "../bfc/wnds/checkbox.h"
#include "../common/script/c_script/h_guiobject.h"
#include "../common/script/objcontroller.h"
#include "../studio/api.h"
#include "../bfc/accessible.h"

#define  SCRIPTCHECKBOX_PARENT CheckBox

// -----------------------------------------------------------------------
// Your wnd object class
class ScriptCheckBox : public SCRIPTCHECKBOX_PARENT {
  
  public:

    ScriptCheckBox();
    virtual ~ScriptCheckBox();

    // XuiObject automatically calls this back for all parameters registered using addParam
    // encountered in the xml source
    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual void onToggle();

    virtual int accessibility_getRole();
    virtual int accessibility_getState() { return CHECKBOX_PARENT::accessibility_getState() | (isActivated() ? STATE_SYSTEM_CHECKED : 0); }

  private:

    // a list of IDs for our xml attributes, we use them in addParam() in the constructor
    enum {
       SCRIPTCHECKBOX_TEXT = 0,
       SCRIPTCHECKBOX_RADIOID,
       SCRIPTCHECKBOX_RADIOVAL,
       SCRIPTCHECKBOX_ACTION,
       SCRIPTCHECKBOX_ACTIONPARAM,
       SCRIPTCHECKBOX_ACTIONTARGET,
    };
    int myxuihandle;
};

// -----------------------------------------------------------------------------------------------------
class CheckBoxController : public ScriptObjectControllerI {
  public:

    virtual const char *getClassName() { return "Checkbox"; }
    virtual const char *getAncestorClassName() { return "GuiObject"; }
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid() { return checkBoxGuid; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  public:
    static scriptVar onToggle(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar newstate);
    static scriptVar setChecked(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar state);
    static scriptVar isChecked(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar setText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar text);
    static scriptVar getText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

  private:

    static function_descriptor_struct exportedFunction[];
};

extern CheckBoxController *checkBoxController;


// -----------------------------------------------------------------------
// This defines the svc_xuiObject that exposes your wnd object

extern char ScriptCheckBoxXuiObjectStr[];
extern char ScriptCheckBoxXuiSvcName[];
class ScriptCheckBoxXuiSvc : public XuiObjectSvc<ScriptCheckBox, ScriptCheckBoxXuiObjectStr, ScriptCheckBoxXuiSvcName> {};

#endif
