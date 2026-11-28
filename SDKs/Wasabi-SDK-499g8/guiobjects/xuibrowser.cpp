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
#include "xuibrowser.h"

char browserXuiObjectStr[] = "Browser"; // This is the xml tag
char browserXuiSvcName[] = "Browser XuiObject Service"; // this is the name of the xuiservice

ScriptBrowserWnd::ScriptBrowserWnd() {
  getScriptObject()->vcpu_setInterface(browserGuid, (void *)static_cast<ScriptBrowserWnd *>(this));
  getScriptObject()->vcpu_setClassName("Browser"); // this is the script class name
  getScriptObject()->vcpu_setController(browserController);

  myxuihandle = newXuiHandle();

  addParam(myxuihandle, "url", BROWSER_SETURL, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "mainmb", BROWSER_SETMAINMB, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "targetname", BROWSER_SETTARGETNAME, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "scrollbars", BROWSER_SETSCROLLBARS, XUI_ATTRIBUTE_IMPLIED);
}

ScriptBrowserWnd::~ScriptBrowserWnd() {
  if (WASABI_API_MAKI->maki_getObjectAtom(MAIN_BROWSER_ATOM_NAME) == getScriptObject())
    WASABI_API_MAKI->maki_setObjectAtom(MAIN_BROWSER_ATOM_NAME, NULL);
}

int ScriptBrowserWnd::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTBROWSERWND_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);
    
  switch (xmlattributeid) {
    case BROWSER_SETURL:
      setUrl(value);
      break;
    case BROWSER_SETMAINMB:
      setMainMB(ATOI(value));
      break;
    case BROWSER_SETTARGETNAME:
      setTargetName(value);
      break;
    case BROWSER_SETSCROLLBARS:
      setScrollbarsFlag(translateScrollbarFlag(value));
      break;
    default:
      return 0;
  }
  return 1;
}

int ScriptBrowserWnd::translateScrollbarFlag(const char *scrollbarflag) {
  if (STRCASEEQLSAFE(scrollbarflag, "auto")) return MiniBrowser::BROWSER_SCROLLBARS_AUTO;
  if (STRCASEEQLSAFE(scrollbarflag, "never")) return MiniBrowser::BROWSER_SCROLLBARS_NEVER;
  if (STRCASEEQLSAFE(scrollbarflag, "always")) return MiniBrowser::BROWSER_SCROLLBARS_ALWAYS;
  if (STRCASEEQLSAFE(scrollbarflag, "default")) return MiniBrowser::BROWSER_SCROLLBARS_DEFAULT; // as specified by HTML content
  return MiniBrowser::BROWSER_SCROLLBARS_AUTO;
}

void ScriptBrowserWnd::onSetVisible(int v) {
  SCRIPTBROWSERWND_PARENT::onSetVisible(v);
  if (v && !defurl.isempty() && !STRCASEEQL(defurl, getCurrentUrl() ? getCurrentUrl() : "")) navigateUrl(defurl);
}

void ScriptBrowserWnd::setMainMB(int m) {
  if (m)  
    WASABI_API_MAKI->maki_setObjectAtom(MAIN_BROWSER_ATOM_NAME, getScriptObject());
  else
    if (WASABI_API_MAKI->maki_getObjectAtom(MAIN_BROWSER_ATOM_NAME) == getScriptObject())
      WASABI_API_MAKI->maki_setObjectAtom(MAIN_BROWSER_ATOM_NAME, NULL);
}

void ScriptBrowserWnd::setUrl(const char *url) {
  defurl = url;
  if (isVisible()) {
    if (!defurl.isempty()) 
      navigateUrl(defurl);
  } 
}

int ScriptBrowserWnd::navigateUrl(const char *url) {
  defurl = url;
  return SCRIPTBROWSERWND_PARENT::navigateUrl(url);
}

int ScriptBrowserWnd::onBeforeNavigate(const char *url, int flags, const char *frame) {
  if (SCRIPTBROWSERWND_PARENT::onBeforeNavigate(url, flags, frame)) return 1;
  scriptVar v = BrowserScriptController::browser_onBeforeNavigate(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_STRING(url), MAKE_SCRIPT_INT(flags), MAKE_SCRIPT_STRING(frame));
  if (v.type != SCRIPT_VOID) 
    return GET_SCRIPT_BOOLEAN(v);
  return 0;
}

void ScriptBrowserWnd::onDocumentComplete(const char *url) {
  SCRIPTBROWSERWND_PARENT::onDocumentComplete(url);
  BrowserScriptController::browser_onDocumentComplete(SCRIPT_CALL, getScriptObject(), MAKE_SCRIPT_STRING(url));
}

// -----------------------------------------------------------------------
// Script Object

BrowserScriptController _browserController;
BrowserScriptController *browserController = &_browserController;

// -- Functions table -------------------------------------
function_descriptor_struct BrowserScriptController::exportedFunction[] = {
  {"gotoUrl",            1, (void*)BrowserScriptController::browser_navigateUrl },
  {"navigateUrl",        1, (void*)BrowserScriptController::browser_navigateUrl },
  {"back",               0, (void*)BrowserScriptController::browser_back },
  {"forward",            0, (void*)BrowserScriptController::browser_forward },
  {"home",               0, (void*)BrowserScriptController::browser_home},
  {"stop",               0, (void*)BrowserScriptController::browser_stop},
  {"refresh",            0, (void*)BrowserScriptController::browser_refresh},
  {"setTargetName",      1, (void*)BrowserScriptController::browser_setTargetName},
  {"onBeforeNavigate",   3, (void*)BrowserScriptController::browser_onBeforeNavigate},
  {"onDocumentComplete", 1, (void*)BrowserScriptController::browser_onDocumentComplete},
};

ScriptObject *BrowserScriptController::instantiate() {
  ScriptBrowserWnd *sb = new ScriptBrowserWnd;
  ASSERT(sb != NULL);
  return sb->getScriptObject();
}

void BrowserScriptController::destroy(ScriptObject *o) {
  ScriptBrowserWnd *sb = static_cast<ScriptBrowserWnd *>(o->vcpu_getInterface(browserGuid));
  ASSERT(sb != NULL);
  delete sb;
}

void *BrowserScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for browsers yet
}

void BrowserScriptController::deencapsulate(void *o) {
}

int BrowserScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *BrowserScriptController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar BrowserScriptController::browser_navigateUrl(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar url) {
  SCRIPT_FUNCTION_INIT
  ASSERT(url.type == SCRIPT_STRING);
  ScriptBrowserWnd *sb = static_cast<ScriptBrowserWnd*>(o->vcpu_getInterface(browserGuid));
  if (sb) sb->navigateUrl(url.data.sdata);
  RETURN_SCRIPT_VOID;  
}

scriptVar BrowserScriptController::browser_back(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptBrowserWnd *sb = static_cast<ScriptBrowserWnd*>(o->vcpu_getInterface(browserGuid));
  if (sb) sb->back();
  RETURN_SCRIPT_VOID;  
}

scriptVar BrowserScriptController::browser_forward(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptBrowserWnd *sb = static_cast<ScriptBrowserWnd*>(o->vcpu_getInterface(browserGuid));
  if (sb) sb->forward();
  RETURN_SCRIPT_VOID;  
}

scriptVar BrowserScriptController::browser_home(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptBrowserWnd *sb = static_cast<ScriptBrowserWnd*>(o->vcpu_getInterface(browserGuid));
  if (sb) sb->home();
  RETURN_SCRIPT_VOID;  
}

scriptVar BrowserScriptController::browser_refresh(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptBrowserWnd *sb = static_cast<ScriptBrowserWnd*>(o->vcpu_getInterface(browserGuid));
  if (sb) sb->refresh();
  RETURN_SCRIPT_VOID;  
}

scriptVar BrowserScriptController::browser_stop(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  ScriptBrowserWnd *sb = static_cast<ScriptBrowserWnd*>(o->vcpu_getInterface(browserGuid));
  if (sb) sb->stop();
  RETURN_SCRIPT_VOID;  
}

scriptVar BrowserScriptController::browser_setTargetName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar name) {
  SCRIPT_FUNCTION_INIT
  ScriptBrowserWnd *sb = static_cast<ScriptBrowserWnd*>(o->vcpu_getInterface(browserGuid));
  if (sb) sb->setTargetName(GET_SCRIPT_STRING(name));
  RETURN_SCRIPT_VOID;  
}

scriptVar BrowserScriptController::browser_onBeforeNavigate(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar url, scriptVar flags, scriptVar framename) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS3(o, browserController, url, flags, framename);
  SCRIPT_FUNCTION_CHECKABORTEVENT;    
  SCRIPT_EXEC_EVENT3(o, url, flags, framename);
}

scriptVar BrowserScriptController::browser_onDocumentComplete(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar url) {
  SCRIPT_FUNCTION_INIT
  PROCESS_HOOKS1(o, browserController, url);
  SCRIPT_FUNCTION_CHECKABORTEVENT;
  SCRIPT_EXEC_EVENT1(o, url);
}

