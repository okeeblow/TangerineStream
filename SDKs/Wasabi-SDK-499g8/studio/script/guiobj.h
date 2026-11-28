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
#ifndef __GUIOBJ_H
#define __GUIOBJ_H

#include <bfc/timerclient.h>
#include <bfc/rootwnd.h>
#include <bfc/string.h>
#include <bfc/timerclient.h>
#include <bfc/depview.h>
#include <common/xmlobject.h>
#include <common/script/guiobject.h>
#ifdef WASABI_COMPILE_CONFIG
#include <bfc/attribs/cfgitem.h>
#endif
#include <common/guiobjwnd.h>

class Layout;
class FindObjectCallback;
class ScriptObject;
class SkinCursor;

#ifndef _REDOCK_STRUCT_DEFINED
#define _REDOCK_STRUCT_DEFINED
typedef struct {
  Layout *l;
  RECT original_rect;
} redock_struct;
#endif

class GuiObjectScriptController : public ScriptObjectControllerI {
  public:

    virtual const char *getClassName();
    virtual const char *getAncestorClassName();
    virtual ScriptObjectController *getAncestorController();
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid();
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);
    virtual int getInstantiable();

  private:

    static function_descriptor_struct exportedFunction[];
    
};

extern GuiObjectScriptController *guiController;

#define GUIOBJECT_SCRIPTPARENT RootObject

#define TARGET_FROZEN 0
#define TARGET_RUNNING 1
#define TARGETTIMER_ID  0x1879

#define ANCHOR_NONE   0
#define ANCHOR_LEFT   1
#define ANCHOR_TOP    2
#define ANCHOR_RIGHT  4
#define ANCHOR_BOTTOM 8

class GuiObjectI;

class GuiObjectTimer : public TimerClientDI {
  
  public:

    GuiObjectTimer() { obj = NULL; }
    virtual ~GuiObjectTimer() { }

  public:
    virtual void timerclient_timerCallback(int id);
    virtual void setGuiObjectI(GuiObjectI *o) { obj = o; }

  private:
    GuiObjectI *obj;
};

#ifdef WASABI_COMPILE_CONFIG
class GuiObjectI : public GuiObject, public DependentViewerTPtr<CfgItem> {
#else
class GuiObjectI : public GuiObject {
#endif
public:

  GuiObjectI(ScriptObject *o);
  virtual ~GuiObjectI();

  virtual RootWnd *guiobject_getRootWnd();
  virtual void guiobject_setRootWnd(RootWnd *w);
  virtual void guiobject_onInit();

  virtual void guiobject_getGuiPosition(int *x, int *y, int *w, int *h, int *rx, int *ry, int *rw, int *rh);
  virtual void guiobject_setGuiPosition(int *x, int *y, int *w, int *h, int *rx, int *ry, int *rw, int *rh);
  virtual int guiobject_getAnchoragePosition(int *x1, int *y1, int *x2, int *y2, int *anchor);
  virtual void guiobject_setAnchoragePosition(int *x1, int *y1, int *x2, int *y2, int *anchor);
  virtual void guiobject_validateAnchorage();

  virtual void guiobject_setParentGroup(Group *g);
  virtual ScriptObject *guiobject_getScriptObject();
  virtual RootObject *guiobject_getRootObject();
  virtual Group *guiobject_getParentGroup();
  virtual GuiObject *guiobject_getParent();
  virtual void guiobject_setTabOrder(int a);

  virtual void guiobject_setId(const char *id); 
  virtual const char *guiobject_getId(); 

	virtual void guiobject_setTargetX(int tx);
	virtual void guiobject_setTargetY(int ty);
	virtual void guiobject_setTargetW(int tw);
	virtual void guiobject_setTargetH(int th);
	virtual void guiobject_setTargetA(int th);
	virtual void guiobject_setTargetSpeed(float s);
	virtual void guiobject_gotoTarget(void);
  virtual void guiobject_cancelTarget();
  virtual void guiobject_reverseTarget(int reverse);

	virtual int guiobject_getAutoWidth();
	virtual int guiobject_getAutoHeight();
  virtual int guiobject_movingToTarget();

  virtual void guiobject_bringToFront();
  virtual void guiobject_bringToBack();
  virtual void guiobject_bringAbove(GuiObject *o);
  virtual void guiobject_bringBelow(GuiObject *o);

  virtual void guiobject_setClickThrough(int ct);
  virtual int guiobject_isClickThrough();

  virtual void guiobject_setAutoSysMetricsX(int a);
  virtual void guiobject_setAutoSysMetricsY(int a);
  virtual void guiobject_setAutoSysMetricsW(int a);
  virtual void guiobject_setAutoSysMetricsH(int a);
  virtual int guiobject_getAutoSysMetricsX();
  virtual int guiobject_getAutoSysMetricsY();
  virtual int guiobject_getAutoSysMetricsW();
  virtual int guiobject_getAutoSysMetricsH();

  virtual int guiobject_getRegionOp();
  virtual void guiobject_setRegionOp(int op);
  virtual int guiobject_isRectRgn();
  virtual void guiobject_setRectRgn(int rrgn);

	virtual void guiobject_onLeftButtonDown(int x, int y);
	virtual void guiobject_onLeftButtonUp(int x, int y);
	virtual void guiobject_onRightButtonDown(int x, int y);
	virtual void guiobject_onRightButtonUp(int x, int y);
	virtual void guiobject_onLeftButtonDblClk(int x, int y);
	virtual void guiobject_onRightButtonDblClk(int x, int y);
	virtual void guiobject_onMouseMove(int x, int y);
	virtual void guiobject_onEnterArea();
	virtual void guiobject_onLeaveArea();
	virtual void guiobject_onEnable(int en);
  virtual void guiobject_setEnabled(int en);
	virtual void guiobject_onResize(int x, int y, int w, int h);
  virtual void guiobject_onSetVisible(int v);
  virtual void guiobject_onTargetReached();
	virtual void guiobject_setAlpha(int a);
	virtual void guiobject_setActiveAlpha(int a);
	virtual void guiobject_setInactiveAlpha(int a);
	virtual int guiobject_getAlpha();
	virtual int guiobject_getActiveAlpha();
	virtual int guiobject_getInactiveAlpha();
	virtual int guiobject_isActive();
  virtual void guiobject_onStartup();
  virtual int guiobject_setXmlParam(const char *param, const char *value);
  virtual const char *guiobject_getXmlParam(const char *param);
  virtual int guiobject_setXmlParamById(int id, const char *value);
  virtual svc_xuiObject *guiobject_getXuiService();
  virtual void guiobject_setXuiService(svc_xuiObject *svc);
  virtual waServiceFactory *guiobject_getXuiServiceFactory();
  virtual void guiobject_setXuiServiceFactory(waServiceFactory *fac);
  virtual GuiObject *guiobject_getTopParent();
#ifdef WASABI_COMPILE_WNDMGR
  virtual Layout *guiobject_getParentLayout();
  virtual int guiobject_runModal();
  virtual void guiobject_endModal(int retcode);
  virtual void guiobject_popParentLayout();
  virtual void guiobject_registerStatusCB(GuiStatusCallback *cb);
  virtual void guiobject_setStatusText(const char *txt, int overlay=FALSE);
  virtual void guiobject_addAppCmds(AppCmds *commands);
  virtual void guiobject_removeAppCmds(AppCmds *commands);
  virtual void guiobject_pushCompleted(int max=100);
  virtual void guiobject_incCompleted(int add=1);
  virtual void guiobject_setCompleted(int pos);
  virtual void guiobject_popCompleted();
#endif
  virtual GuiObject *guiobject_findObject(const char *id);
  virtual GuiObject *guiobject_findObjectXY(int x, int y); // in client coordinates relative to this object
  virtual GuiObject *guiobject_findObjectByInterface(GUID interface_guid);
  virtual GuiObject *guiobject_findObjectByCallback(FindObjectCallback *cb);
  virtual void guiobject_setMover(int m);
  virtual int guiobject_getMover();
  virtual void guiobject_onCancelCapture();
  virtual void guiobject_onChar(unsigned int c);
  virtual void guiobject_onKeyDown(int vkcode);
  virtual void guiobject_onKeyUp(int vkcode);
  
  virtual FOURCC guiobject_getDropTarget();
  virtual void guiobject_setDropTarget(const char *strval);

	virtual void onTargetTimer();

#ifdef WASABI_COMPILE_CONFIG
  virtual void guiobject_setCfgAttrib(CfgItem *item, const char *name);

  virtual int viewer_onEvent(CfgItem *item, int event, int param, void *ptr, int ptrlen);
  virtual CfgItem *guiobject_getCfgItem();
  const char *guiobject_getCfgAttrib();

  virtual int guiobject_getCfgInt();
  virtual void guiobject_setCfgInt(int i);
  virtual float guiobject_getCfgFloat();
  virtual void guiobject_setCfgFloat(float f);
  virtual const char *guiobject_getCfgString();
  virtual void guiobject_setCfgString(const char *s);
  virtual int guiobject_hasCfgAttrib();
#endif

  virtual const char *guiobject_getName();
  virtual void guiobject_onAccelerator(const char *accel);
  virtual int guiobject_onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source);

  virtual int guiobject_wantFocus();
  virtual void guiobject_setNoDoubleClick(int no);
  virtual void guiobject_setNoLeftClick(int no);
  virtual void guiobject_setNoRightClick(int no);
  virtual void guiobject_setNoMouseMove(int no);
  virtual void guiobject_setNoContextMenu(int no);

  virtual void guiobject_setCursor(const char *c);

  protected:

  RECVS_DISPATCH;

  private:
#ifdef WASABI_COMPILE_CONFIG
    void setCfgAttr(const char *strvalue);
    virtual void dataChanged();
#endif

    void infoMenu(GuiObject *o, int x, int y);

    int targetx, targety, targetw, targeth, targeta, targetspeed;
    int start_time;
    int targetstatus;

	  void startTargetTimer();
	  void stopTargetTimer();
	  void ensureTimerOk();

    int autosysmetricsx, autosysmetricsy, autosysmetricsw, autosysmetricsh;

	  int startx, starty, startw, starth, starta;

    int in_area;
    String guiobject_id;
    int clickthrough;
    String autonotify;
    RootWnd *my_root_wnd;
    ScriptObject *my_script_object;
    GuiObjectTimer timer;

	  int gui_x;
	  int gui_y;
	  int gui_w;
	  int gui_h;
	  int gui_rx;
	  int gui_ry;
	  int gui_rw;
	  int gui_rh;
    int mover, moving;
  FOURCC droptarget;
    Group *p_group;
    POINT anchor;
    svc_xuiObject *xuisvc;
    waServiceFactory *xuifac;
    PtrList<String> notifylist;
    int wantfocus;
    int anchor_x1, anchor_y1, anchor_x2, anchor_y2, anchorage, anchorage_invalidated;
    int reversetarget;
    SkinCursor *cursor;
    redock_struct redock;
#ifdef WASABI_COMPILE_WNDMGR
    String cfgattrname;
    String cfgguid;
    CfgItem *cfgitem;
#endif
};




class GuiObject_ScriptMethods {
  public:

  static scriptVar getId(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
  static scriptVar show(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
  static scriptVar hide(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
  static scriptVar isvisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
  static scriptVar getAlpha(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
  static scriptVar setAlpha(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a); 
  static scriptVar getActiveAlpha(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
  static scriptVar setActiveAlpha(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a); 
  static scriptVar getInactiveAlpha(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
  static scriptVar setInactiveAlpha(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a); 
  static scriptVar resize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y, scriptVar w, scriptVar h); 
  static scriptVar onResize(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y, scriptVar w, scriptVar h);
  static scriptVar init(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar parentGroup);
  static scriptVar bringToFront(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar bringToBack(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar bringAbove(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar obj);
  static scriptVar bringBelow(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar obj);
  static scriptVar getIdVar(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar onLeftButtonDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar onLeftButtonUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar onRightButtonDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar onRightButtonUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar onRightButtonDblClk(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar onLeftButtonDblClk(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar onMouseMove(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar onEnterArea(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar onLeaveArea(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar onChar(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar c);
  static scriptVar onKeyDown(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar c);
  static scriptVar onKeyUp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar c);
  static scriptVar setEnabled(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a);
  static scriptVar getEnabled(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar onEnable(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar v);
  static scriptVar isMouseOver(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar getLeft(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getTop(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getWidth(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getHeight(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar setTargetX(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a);
  static scriptVar setTargetY(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a);
  static scriptVar setTargetW(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a);
  static scriptVar setTargetH(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a);
  static scriptVar setTargetA(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a);
  static scriptVar setTargetSpeed(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar a);
  static scriptVar gotoTarget(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
  static scriptVar onTargetReached(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar cancelTarget(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar reverseTarget(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar r);
  static scriptVar movingToTarget(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar setXmlParam(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar param, scriptVar value);
  static scriptVar getXmlParam(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar param);
  static scriptVar onSetVisible(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar v);
  static scriptVar onStartup(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getGuiX(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getGuiY(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getGuiW(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getGuiH(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getGuiRelatX(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getGuiRelatY(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getGuiRelatW(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getGuiRelatH(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar isActive(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getParent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getTopParent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
#ifdef WASABI_COMPILE_WNDMGR
  static scriptVar getParentLayout(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar runModal(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar endModal(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar retcode);
  static scriptVar popParentLayout(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar setStatusText(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar text, scriptVar overlay);
#endif
  static scriptVar findObject(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar id);
  static scriptVar findObjectXY(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x, scriptVar y);
  static scriptVar getName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getMover(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar setMover(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar is);
  static scriptVar setDropTarget(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar targ);
#ifdef WASABI_COMPILE_CONFIG
  static scriptVar onCfgChanged(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar cfgGetInt(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar cfgSetInt(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar v);
  static scriptVar cfgGetString(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar cfgSetString(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar v);
  static scriptVar cfgGetFloat(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar cfgSetFloat(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar v);
  static scriptVar cfgGetGuid(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar cfgGetAttributeName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
#endif
  static scriptVar clientToScreenX(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x);
  static scriptVar clientToScreenY(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar y);
  static scriptVar clientToScreenW(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x);
  static scriptVar clientToScreenH(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar y);
  static scriptVar screenToClientX(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x);
  static scriptVar screenToClientY(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar y);
  static scriptVar screenToClientW(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar x);
  static scriptVar screenToClientH(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar y);
  static scriptVar getAutoWidth(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getAutoHeight(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar setFocus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar onGetFocus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar onKillFocus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar onAccelerator(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar accel);
  static scriptVar sendAction(SCRIPT_FUNCTION_PARAMS, ScriptObject *obj, scriptVar action, scriptVar param, scriptVar x, scriptVar y, scriptVar p1, scriptVar p2);
  static scriptVar onAction(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar action, scriptVar param, scriptVar x, scriptVar y, scriptVar p1, scriptVar p2, scriptVar source);
  static scriptVar isMouseOverRect(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar getInterface(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar interfaceguid);
};

extern char guiobjectXuiStr[];
extern char guiobjectXuiSvcName[];
class GuiObjectXuiSvc : public XuiObjectSvc<GuiObjectWnd, guiobjectXuiStr, guiobjectXuiSvcName> {};


#endif
