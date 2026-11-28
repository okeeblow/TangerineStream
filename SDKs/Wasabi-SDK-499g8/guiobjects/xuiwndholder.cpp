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
#include "xuiwndholder.h"
#include "../bfc/canvas.h"
#include "../bfc/paramparser.h"
#include "../studio/script/sregion.h"

// -----------------------------------------------------------------------
char WindowHolderXuiObjectStr[] = "WindowHolder"; // This is the xml tag
char WindowHolderXuiObjectStrCompat[] = "Component"; // This is the old xml tag

char WindowHolderXuiSvcName[] = "XuiWindowHolder XuiObject Service";

// {7DB51C8C-36C0-4585-9240-A2DB22B1B8F7}
static const GUID pvt_xuiWindowHolder = 
{ 0x7db51c8c, 0x36c0, 0x4585, { 0x92, 0x40, 0xa2, 0xdb, 0x22, 0xb1, 0xb8, 0xf7 } };

// -----------------------------------------------------------------------
XuiWindowHolder::XuiWindowHolder() {
  getScriptObject()->vcpu_setInterface(windowHolderGuid, (void *)static_cast<WindowHolder *>(this));
  getScriptObject()->vcpu_setInterface(pvt_xuiWindowHolder, (void *)static_cast<XuiWindowHolder *>(this));
  getScriptObject()->vcpu_setClassName("WindowHolder"); // this is the script class name
  getScriptObject()->vcpu_setController(windowHolderController);

  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "param", XUIWNDHOLDER_ADDHOLD, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "component", XUIWNDHOLDER_ADDHOLD, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "hold", XUIWNDHOLDER_ADDHOLD, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "noshowcmdbar", XUIWNDHOLDER_SETNOSHOWCMDBAR, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "noanimatedrects", XUIWNDHOLDER_SETNOANIMRECTS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "disableanimatedrects", XUIWNDHOLDER_SETNOANIMRECTS, XUI_ATTRIBUTE_IMPLIED); 
  addParam(myxuihandle, "autoopen", XUIWNDHOLDER_SETAUTOOPEN, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "autoclose", XUIWNDHOLDER_SETAUTOCLOSE, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "autofocus", XUIWNDHOLDER_SETAUTOFOCUS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "autoavailable", XUIWNDHOLDER_SETAUTOAVAILABLE, XUI_ATTRIBUTE_IMPLIED);

  setXmlParam("autofocus", "1");
}

XuiWindowHolder::~XuiWindowHolder() {
}

void XuiWindowHolder::setRegionFromMap(ScriptObject *map, int byte, int inverse) {
}

void XuiWindowHolder::setRegion(ScriptObject *_region) {
#pragma CHAT("bas", "lone", "I implemented setRegion for ya once SRegion is safe")
#if 0// if SRegion were cross-dll safe this would work
  const GUID regionGuid = 
{ 0x3a370c02, 0x3cbf, 0x439f, { 0x84, 0xf1, 0x86, 0x88, 0x5b, 0xcf, 0x1e, 0x36 } };
  void *reg = _region->vcpu_getInterface(regionGuid);
  ASSERT(reg != NULL);
  SRegion *sr = static_cast<SRegion*>(reg);
  ASSERT(sr != NULL);
  Region *region = sr->getRegion();
  ASSERT(region != NULL);
  Region *clone = region->clone();
  clone->scale(getRenderRatio(), getRenderRatio());
  SetWindowRgn(getCurRootWnd()->gethWnd(), clone->makeWindowRegion(), TRUE);
  region->disposeClone(clone);
#endif
}

// -----------------------------------------------------------------------
int XuiWindowHolder::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return XUIWNDHOLDER_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case XUIWNDHOLDER_ADDHOLD: 
      if (STRCASEEQL(value, "@ALL@") || STRCASEEQL(value, "guid:default")) {
        setAcceptAllGuids(1);
        setAcceptAllGroups(1);
      } else {
        setAcceptAllGuids(0);
        setAcceptAllGroups(0);
        ParamParser pp(value);
        for (int i=0;i<pp.getNumItems();i++) {
          const char *e = pp.enumItem(i);
          if (*e == '{' || !STRNINCMP(e, "guid:")) {
            GUID *g = parseGUID(e);
            if (g)
              addAcceptGuid(*g);
          }
          else
            addAcceptGroup(e);
        }
      }
      break;
    case XUIWNDHOLDER_SETNOSHOWCMDBAR:
      setNoCmdBar(ATOI(value));
      break;
    case XUIWNDHOLDER_SETNOANIMRECTS:
      setNoAnim(ATOI(value));
      break;
    case XUIWNDHOLDER_SETAUTOOPEN:
      setAutoOpen(ATOI(value));
      break;
    case XUIWNDHOLDER_SETAUTOCLOSE:
      setAutoClose(ATOI(value));
      break;
    case XUIWNDHOLDER_SETAUTOFOCUS:
      setAutoFocus(ATOI(value));
      break;
    case XUIWNDHOLDER_SETAUTOAVAILABLE:
      setAutoAvailable(ATOI(value));
      break;
    default:
      return 0;
  }
  return 1;
}

GUID *XuiWindowHolder::parseGUID(const char *id) {
  if (STRNICMP(id,"guid:{",6)==0) {
    static GUID g;
    id+=5;
    g = nsGUID::fromChar(id);
    return &g;
  }
  if (id && *id == '{') {
    static GUID g;
    g = nsGUID::fromChar(id);
    return &g;
  }                                             
  if(STRICMP(id,"guid:avs")==0) {
    static GUID g={ 10, 12, 16, { 255, 123, 1, 1, 66, 99, 69, 12 } };
    return &g;
  }
  if (STRICMP(id,"guid:pl")==0 || STRICMP(id,"guid:playlist")==0) {
    static GUID g={ 0x45f3f7c1, 0xa6f3, 0x4ee6, { 0xa1, 0x5e, 0x12, 0x5e, 0x92, 0xfc, 0x3f, 0x8d } };
    return &g;
  }
  if (STRICMP(id,"guid:ml")==0 || STRICMP(id,"guid:musiclibrary")==0 || STRICMP(id,"guid:library")==0) {
    static GUID g={ 0x6b0edf80, 0xc9a5, 0x11d3, { 0x9f, 0x26, 0x00, 0xc0, 0x4f, 0x39, 0xff, 0xc6 } };
    return &g;
  }
  return NULL;
}

// -----------------------------------------------------------------------
// Script Object

WindowHolderScriptController _windowHolderController;
WindowHolderScriptController *windowHolderController = &_windowHolderController;

// -- Functions table -------------------------------------
function_descriptor_struct WindowHolderScriptController::exportedFunction[] = {
  {"getGUID", 1, (void*)WindowHolderScriptController::script_getGUID },
  {"setRegionFromMap", 3, (void*)WindowHolderScriptController::script_setRegionFromMap },
  {"setRegion", 1, (void*)WindowHolderScriptController::script_setRegion },
  {"getContent", 0, (void*)WindowHolderScriptController::script_getContent},
};

ScriptObject *WindowHolderScriptController::instantiate() {
  XuiWindowHolder *wh = new XuiWindowHolder;
  ASSERT(wh != NULL);
  return wh->getScriptObject();
}

void WindowHolderScriptController::destroy(ScriptObject *o) {
  XuiWindowHolder *wh = static_cast<XuiWindowHolder *>(o->vcpu_getInterface(pvt_xuiWindowHolder));
  ASSERT(wh != NULL);
  delete wh;
}

void *WindowHolderScriptController::encapsulate(ScriptObject *o) {
  return NULL; // no encapsulation for windowholder yet
}

void WindowHolderScriptController::deencapsulate(void *o) {
}

int WindowHolderScriptController::getNumFunctions() { 
  return sizeof(exportedFunction) / sizeof(function_descriptor_struct); 
}

const function_descriptor_struct *WindowHolderScriptController::getExportedFunctions() { 
  return exportedFunction; 
}


scriptVar WindowHolderScriptController::script_getGUID(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar url) {
  SCRIPT_FUNCTION_INIT
  ASSERT(url.type == SCRIPT_STRING);
  XuiWindowHolder *wh = static_cast<XuiWindowHolder*>(o->vcpu_getInterface(pvt_xuiWindowHolder));
  if (wh) return MAKE_SCRIPT_STRING(wh->getCurId());
  return MAKE_SCRIPT_STRING("");  
}

scriptVar WindowHolderScriptController::script_setRegionFromMap(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar map, scriptVar byte, scriptVar inv) {
  SCRIPT_FUNCTION_INIT
  XuiWindowHolder *xu = static_cast<XuiWindowHolder*>(o->vcpu_getInterface(pvt_xuiWindowHolder));
  if (xu) xu->setRegionFromMap(GET_SCRIPT_OBJECT(map), GET_SCRIPT_INT(byte), GET_SCRIPT_INT(inv));
  RETURN_SCRIPT_VOID;  
}

scriptVar WindowHolderScriptController::script_setRegion(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar reg) {
  SCRIPT_FUNCTION_INIT
  XuiWindowHolder *xu= static_cast<XuiWindowHolder*>(o->vcpu_getInterface(pvt_xuiWindowHolder));
  if (xu) xu->setRegion(GET_SCRIPT_OBJECT(reg));
  RETURN_SCRIPT_VOID;  
}

scriptVar WindowHolderScriptController::script_getContent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
  SCRIPT_FUNCTION_INIT
  XuiWindowHolder *xu= static_cast<XuiWindowHolder*>(o->vcpu_getInterface(pvt_xuiWindowHolder));
  if (xu) {
    RootWnd *w = xu->getRootWndPtr();
    if (w) {
      GuiObject *o = w->getGuiObject();
      if (o) 
        return MAKE_SCRIPT_OBJECT(o->guiobject_getScriptObject());
    }
  }
  RETURN_SCRIPT_NULL;
}
