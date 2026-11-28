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
#include "../bfc/api/api_maki.h"
#include "guiobjwnd.h"
#include "script/scriptobj.h"
#include "script/scriptguid.h"
#include "../studio/services/svc_droptarget.h"
#include "../bfc/notifmsg.h"
#include "../studio/script/rootobject.h"

GuiObjectWnd::GuiObjectWnd() {
  my_gui_object = static_cast<GuiObject *>(WASABI_API_MAKI->maki_encapsulate(guiObjectGuid, getScriptObject()));
  getScriptObject()->vcpu_setInterface(xmlObjectGuid, static_cast<XmlObject *>(this));
  getScriptObject()->vcpu_setInterface(guiObjectWndGuid, static_cast<GuiObjectWnd *>(this));
  my_gui_object->guiobject_setRootWnd(this);
  getScriptObject()->vcpu_setClassName("GuiObject");
  getScriptObject()->vcpu_setController(WASABI_API_MAKI->maki_getController(guiObjectGuid));
  cfg_reentry = 0;
  xuihandle = newXuiHandle();
  addParam(xuihandle, "id", GUIOBJECT_ID, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "alpha", GUIOBJECT_ALPHA, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "activealpha", GUIOBJECT_ACTIVEALPHA, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "inactivealpha", GUIOBJECT_INACTIVEALPHA, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "sysregion", GUIOBJECT_SYSREGION, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "regionop", GUIOBJECT_SYSREGION, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "rectrgn", GUIOBJECT_RECTRGN, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "tooltip", GUIOBJECT_TOOLTIP, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "sysmetricsx", GUIOBJECT_SYSMETRICSX, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "sysmetricsy", GUIOBJECT_SYSMETRICSY, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "sysmetricsw", GUIOBJECT_SYSMETRICSW, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "sysmetricsh", GUIOBJECT_SYSMETRICSH, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "move", GUIOBJECT_MOVE, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "renderbasetexture", GUIOBJECT_RENDERBASETEXTURE, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "cfgattrib", GUIOBJECT_CFGATTR, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "x", GUIOBJECT_X, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "y", GUIOBJECT_Y, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "w", GUIOBJECT_W, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "h", GUIOBJECT_H, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "visible", GUIOBJECT_VISIBLE, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "enabled", GUIOBJECT_ENABLED, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "relatx", GUIOBJECT_RELATX, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "relaty", GUIOBJECT_RELATY, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "relatw", GUIOBJECT_RELATW, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "relath", GUIOBJECT_RELATH, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "droptarget", GUIOBJECT_DROPTARGET, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "ghost", GUIOBJECT_GHOST, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "notify", GUIOBJECT_NOTIFY, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "taborder", GUIOBJECT_TABORDER, XUI_ATTRIBUTE_IMPLIED);
  for (int i=0;i<10;i++)
    addParam(xuihandle, StringPrintf("notify%d", i), GUIOBJECT_NOTIFY, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "focusonclick", GUIOBJECT_FOCUSONCLICK, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "wantfocus", GUIOBJECT_WANTFOCUS, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "nodblclick", GUIOBJECT_SETNODBLCLICK, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "noleftclick", GUIOBJECT_SETNOLEFTCLICK, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "norightclick", GUIOBJECT_SETNORIGHTCLICK, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "nomousemove", GUIOBJECT_SETNOMOUSEMOVE, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "nocontextmenu", GUIOBJECT_SETNOCONTEXTMENU, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "x1", GUIOBJECT_SETX1, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "y1", GUIOBJECT_SETY1, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "x2", GUIOBJECT_SETX2, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "y2", GUIOBJECT_SETY2, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "anchor", GUIOBJECT_SETANCHOR, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "cursor", GUIOBJECT_SETCURSOR, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "fitparent", GUIOBJECT_FITTOPARENT, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "ncrole", GUIOBJECT_NCROLE, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "userdata", GUIOBJECT_USERDATA, XUI_ATTRIBUTE_IMPLIED);
}

GuiObjectWnd::~GuiObjectWnd() {
  WASABI_API_MAKI->maki_deencapsulate(guiObjectGuid, my_gui_object);
  my_gui_object = NULL;
}

int GuiObjectWnd::onRightButtonDown(int x, int y) {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onRightButtonDown(x, y);
  my_gui_object->guiobject_onRightButtonDown(x, y);
  return 1;
}

int GuiObjectWnd::onRightButtonUp(int x, int y) {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onRightButtonUp(x, y);
  my_gui_object->guiobject_onRightButtonUp(x, y);
  return 1;
}

int GuiObjectWnd::onLeftButtonDown(int x, int y) {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onLeftButtonDown(x, y);
  my_gui_object->guiobject_onLeftButtonDown(x, y);
  return 1;
}

int GuiObjectWnd::onLeftButtonUp(int x, int y) {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onLeftButtonUp(x, y);
  my_gui_object->guiobject_onLeftButtonUp(x, y);
  return 1;
}

int GuiObjectWnd::onMouseMove(int x, int y) {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onMouseMove(x, y);
  my_gui_object->guiobject_onMouseMove(x, y);
  return 1;
}

int GuiObjectWnd::onLeftButtonDblClk(int x, int y) {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onLeftButtonDblClk(x, y);
  my_gui_object->guiobject_onLeftButtonDblClk(x, y);
  return 1;
}

int GuiObjectWnd::onRightButtonDblClk(int x, int y) {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onRightButtonDblClk(x, y);
  my_gui_object->guiobject_onRightButtonDblClk(x, y);
  return 1;
}

int GuiObjectWnd::onResize() {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onResize();
  if (!isInited()) return 1;
  RootWnd *w = my_gui_object->guiobject_getRootWnd();
  RECT r;
  w->getClientRect(&r);
  my_gui_object->guiobject_onResize(r.left, r.top, r.right-r.left, r.bottom-r.top);
  return 1;
}

void GuiObjectWnd::onEnterArea() {
  if (!my_gui_object) return;
  GUIOBJECTWND_PARENT::onEnterArea();
  my_gui_object->guiobject_onEnterArea();
}

void GuiObjectWnd::onLeaveArea() {
  if (!my_gui_object) return;
  GUIOBJECTWND_PARENT::onLeaveArea();
  my_gui_object->guiobject_onLeaveArea();
}

void GuiObjectWnd::onSetVisible(int show) {
  if (!my_gui_object) return;
  GUIOBJECTWND_PARENT::onSetVisible(show);
  my_gui_object->guiobject_onSetVisible(show);
}

int GuiObjectWnd::onEnable(int en) {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onEnable(en);
  my_gui_object->guiobject_onEnable(en);
  return 1;
}

GuiObject *GuiObjectWnd::getGuiObject() {
  return my_gui_object;
}

RootObject *GuiObjectWnd::getRootObject() {
  return my_gui_object->guiobject_getRootObject();
}

int GuiObjectWnd::dragDrop(RootWnd *sourceWnd, int x, int y) {
  int r=DropTargetEnum::throwDrop(my_gui_object->guiobject_getDropTarget(), sourceWnd, x, y);
  if (r == 0) {
    RootWnd *p=getParent();
    if (p != NULL) {
      DragInterface *d=p->getDragInterface();
      if (d != NULL) 
        return d->dragDrop(sourceWnd, x, y);
    }
  }
  return r;
}

int GuiObjectWnd::onActivate() {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onActivate();
  invalidate();
  return 1;
}

int GuiObjectWnd::onDeactivate() {
  if (!my_gui_object) return 1;
  GUIOBJECTWND_PARENT::onDeactivate();
  invalidate();
  return 1;
}

void GuiObjectWnd::onCancelCapture() {
  if (!my_gui_object) return;
  GUIOBJECTWND_PARENT::onCancelCapture();
  my_gui_object->guiobject_onCancelCapture();
}

int GuiObjectWnd::setXuiParam(int _xuihandle, int attrid, const char *name, const char *value) {
  if (_xuihandle == xuihandle) {
    switch (attrid) {
      case GUIOBJECT_FOCUSONCLICK:
        setFocusOnClick(ATOI(value));
      break;
      default:
        getGuiObject()->guiobject_setXmlParamById(attrid, value);
      break;
    }
  }
  return 0;
}

int GuiObjectWnd::onUnknownXmlParam(const char *param, const char *value) {
  return onUnknownXuiParam(param, value);
}

int GuiObjectWnd::setXmlParamById(int xmlhandle, int attrid, const char *name, const char *value) {
  return setXuiParam(xmlhandle, attrid, name, value);
}

void *GuiObjectWnd::getInterface(GUID interface_guid) {
  void *r = GUIOBJECTWND_PARENT::getInterface(interface_guid);
  if (r) return r;
  return getRootObject()->rootobject_getScriptObject()->vcpu_getInterface(interface_guid);
}

int GuiObjectWnd::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source) {
#ifdef WASABI_COMPILE_CONFIG
  if (STRCASEEQL(action, "reload_config") && isInited()) {
    if (cfg_reentry) return 1;
    cfg_reentry = 1;
    int r = onReloadConfig();
    cfg_reentry = 0;
    return r;
  } 
#endif
  int rt = GUIOBJECTWND_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, source);
  getGuiObject()->guiobject_onAction(action, param, x, y, p1, p2, data, datalen, source);
  return rt;
}

void GuiObjectWnd::setContent(const char *groupid_orguid, int autoresizefromcontent) {
//  abstract_setAllowDeferredContent(0);
  abstract_setContent(groupid_orguid, autoresizefromcontent);
}

void GuiObjectWnd::setContentBySkinItem(SkinItem *item, int autoresizefromcontent) {
//  abstract_setAllowDeferredContent(0);
  abstract_setContentBySkinItem(item, autoresizefromcontent);
}

void GuiObjectWnd::abstract_onNewContent() {
  GUIOBJECTWND_PARENT::abstract_onNewContent();
  onNewContent();
#ifdef WASABI_COMPILE_CONFIG
  if (getGuiObject()->guiobject_hasCfgAttrib())
    onReloadConfig();
#endif
}

//#include "profiler.h"
GuiObject *GuiObjectWnd::findObject(const char *object_id) {
//  PR_ENTER(StringPrintf("findObject %s", object_id));
  return getGuiObject()->guiobject_findObject(object_id);
//  PR_LEAVE();
}

#ifdef WASABI_COMPILE_SCRIPT
ScriptObject *GuiObjectWnd::findScriptObject(const char *object_id) {
  GuiObject *fo = getGuiObject()->guiobject_findObject(object_id);
  if (fo != NULL) return fo->guiobject_getScriptObject();
  return NULL;
}
#endif
const char *GuiObjectWnd::getId() {
  if (my_gui_object)
    return my_gui_object->guiobject_getId();
  return GUIOBJECTWND_PARENT::getId();
}

int GuiObjectWnd::onPostOnInit() {
  int r = GUIOBJECTWND_PARENT::onPostOnInit();
#ifdef WASABI_COMPILE_CONFIG
  if (getGuiObject()->guiobject_hasCfgAttrib())
    onReloadConfig();
#endif
  return r;
}

int GuiObjectWnd::onInit() {
  int r = GUIOBJECTWND_PARENT::onInit();
  getGuiObject()->guiobject_onInit();
  return r;
}

int GuiObjectWnd::onChar(unsigned int c) {
  if (!my_gui_object) return 1;
  int r = GUIOBJECTWND_PARENT::onChar(c);
  getGuiObject()->guiobject_onChar(c);
  return r;
}

int GuiObjectWnd::onKeyDown(int vkcode) {
  if (!my_gui_object) return 1;
  int r = GUIOBJECTWND_PARENT::onKeyDown(vkcode);
  getGuiObject()->guiobject_onKeyDown(vkcode);
  return r;
}

int GuiObjectWnd::onKeyUp(int vkcode) {
  if (!my_gui_object) return 1;
  int r = GUIOBJECTWND_PARENT::onKeyUp(vkcode);
  getGuiObject()->guiobject_onKeyUp(vkcode);
  return r;
}

int GuiObjectWnd::onGetFocus() {
  if (!my_gui_object) return 1;
  int r = GUIOBJECTWND_PARENT::onGetFocus();
  getGuiObject()->guiobject_onGetFocus();
  return r;
}

int GuiObjectWnd::onKillFocus() {
  if (!my_gui_object) return 1;
  int r = GUIOBJECTWND_PARENT::onKillFocus();
  getGuiObject()->guiobject_onKillFocus();
  return r;
}

int GuiObjectWnd::onAcceleratorEvent(const char *name) {
  int r = GUIOBJECTWND_PARENT::onAcceleratorEvent(name);
  getGuiObject()->guiobject_onAccelerator(name);
  return r;
}

int GuiObjectWnd::wantFocus() {
  if (GUIOBJECTWND_PARENT::wantFocus()) return 1;
  if (my_gui_object)
    return my_gui_object->guiobject_wantFocus();
  return 0;
}
