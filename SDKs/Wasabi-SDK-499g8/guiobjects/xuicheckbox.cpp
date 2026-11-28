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
#include "precomp.h"
#include "xuicheckbox.h"
#include "../bfc/paramparser.h"
#include "../common/script/c_script/c_text.h"

// -----------------------------------------------------------------------
char ScriptCheckBoxXuiObjectStr[] = "Wasabi:CheckBox"; // This is the xml tag
char ScriptCheckBoxXuiSvcName[] = "Wasabi:CheckBox XuiObject Service"; 

// -----------------------------------------------------------------------
ScriptCheckBox::ScriptCheckBox() : SCRIPTCHECKBOX_PARENT() {
  getScriptObject()->vcpu_setInterface(checkBoxGuid, (void *)static_cast<ScriptCheckBox*>(this));
  getScriptObject()->vcpu_setClassName("CheckBox"); // this is the script class name
  getScriptObject()->vcpu_setController(checkBoxController);

  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "text", SCRIPTCHECKBOX_TEXT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "radioid", SCRIPTCHECKBOX_RADIOID, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "radioval", SCRIPTCHECKBOX_RADIOVAL, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "action", SCRIPTCHECKBOX_ACTION, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "param", SCRIPTCHECKBOX_ACTIONPARAM, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "action_target", SCRIPTCHECKBOX_ACTIONTARGET, XUI_ATTRIBUTE_IMPLIED);

}

// -----------------------------------------------------------------------
ScriptCheckBox::~ScriptCheckBox() {
}

// -----------------------------------------------------------------------
int ScriptCheckBox::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTCHECKBOX_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  // Parcel the values out to the window object we multiply inherit from
  switch (xmlattributeid) {
    case  SCRIPTCHECKBOX_TEXT:
      setText(value);
    break;
    case  SCRIPTCHECKBOX_RADIOID:
      setRadioid(value);
    break;
    case  SCRIPTCHECKBOX_RADIOVAL:
      setRadioVal(value);
    break;
    case  SCRIPTCHECKBOX_ACTION:
      setAction(value);
    break;
    case  SCRIPTCHECKBOX_ACTIONPARAM:
      setActionParam(value);
    break;
    case  SCRIPTCHECKBOX_ACTIONTARGET:
      setActionTarget(value);
    break;
    default:
      return 0;
  }
  return 1;
}

void ScriptCheckBox::onToggle() {
  SCRIPTCHECKBOX_PARENT::onToggle();
  Accessible *a = getAccessibleObject();
  if (a != NULL) {
    a->onStateChange();
  }
  CheckBoxController::onToggle(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_INT(isActivated()));
}

int ScriptCheckBox::accessibility_getRole() {
  const char *a = getRadioid();
  if (a != NULL && *a != 0) return ROLE_SYSTEM_RADIOBUTTON;
  return ROLE_SYSTEM_CHECKBUTTON;
}


// -----------------------------------------------------------------------
// Script Object

CheckBoxController _checkBoxController;
CheckBoxController *checkBoxController = &_checkBoxController;

// -- Functions table -------------------------------------
function_descriptor_struct CheckBoxController::exportedFunction[] = {
  {"onToggle",            1, (void*)CheckBoxController::onToggle},
  {"setChecked",          1, (void*)CheckBoxController::setChecked},
  {"isChecked",           0, (void*)CheckBoxController::isChecked},
  {"setText",             1, (void*)CheckBoxController::setText},
  {"getText",             0, (void*)CheckBoxController::getText},
};

ScriptObject *CheckBoxController::instantiate() {
  ScriptCheckBox *sb = new ScriptCheckBox;
  ASSERT(sb != NULL);
  return sb->getScriptObject();
}

void CheckBoxController::destroy(ScriptObject *o) {
  ScriptCheckBox *sb = static_cast<ScriptCheckBox *>(o->vcpu_getInterface(checkBoxGuid));
  ASSERT(sb != NULL);
  delete sb;
}

void *CheckBoxController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for checkboxes yet
}

void CheckBoxController::deencapsulate(void *o) {
}

int CheckBoxController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *CheckBoxController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar CheckBoxController::onToggle(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar newstate) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, checkBoxController, newstate);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, newstate);
}

scriptVar CheckBoxController::setChecked(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar state) {
  SCRIPT_FUNCTION_INIT
  ScriptCheckBox *sb = static_cast<ScriptCheckBox *>(o->vcpu_getInterface(checkBoxGuid));
  if (sb) sb->setActivated(GET_SCRIPT_INT(state));
  RETURN_SCRIPT_VOID;  
}

scriptVar CheckBoxController::isChecked(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  int a = 0;
  ScriptCheckBox *sb = static_cast<ScriptCheckBox *>(o->vcpu_getInterface(checkBoxGuid));
  if (sb) a = sb->isActivated();
  return MAKE_SCRIPT_INT(a);  
}

scriptVar CheckBoxController::setText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar text) {
  SCRIPT_FUNCTION_INIT
  ScriptCheckBox *sb = static_cast<ScriptCheckBox *>(o->vcpu_getInterface(checkBoxGuid));
  if (sb) sb->setText(GET_SCRIPT_STRING(text));
  RETURN_SCRIPT_VOID;  
}

scriptVar CheckBoxController::getText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  String a;
  ScriptCheckBox *sb = static_cast<ScriptCheckBox *>(o->vcpu_getInterface(checkBoxGuid));
  if (sb) a = sb->getText();
  return MAKE_SCRIPT_STRING(a);  
}
