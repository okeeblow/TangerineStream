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
#ifndef __XUISTATUS_H
#define __XUISTATUS_H

#include "../bfc/wnds/status.h"
#include "../common/script/objcontroller.h"

#define XUISTATUS_PARENT StatusBar
class XuiStatus : public XUISTATUS_PARENT {
public:
  XuiStatus();
  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

  virtual void callme(const char *txt);

private:
  int myxuihandle;
};

extern char StatusXuiObjectStr[];
extern char StatusXuiSvcName[];
class LayoutStatusXuiSvc : public XuiObjectSvc<XuiStatus, StatusXuiObjectStr, StatusXuiSvcName> {};

// -----------------------------------------------------------------------------------------------------
class LayoutStatusController : public ScriptObjectControllerI {
  public:

    virtual const char *getClassName() { return "LayoutStatus"; }
    virtual const char *getAncestorClassName() { return "GuiObject"; }
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid() { return layoutStatusGuid; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  public:
    static scriptVar layoutstatus_callme(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar str);

  private:

    static function_descriptor_struct exportedFunction[];
};

extern LayoutStatusController *layoutStatusController;

#endif
