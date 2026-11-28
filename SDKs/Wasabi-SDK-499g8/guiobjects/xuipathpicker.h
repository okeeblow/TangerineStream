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
#ifndef __XUIPATHPICKER_H
#define __XUIPATHPICKER_H

#include "../bfc/wnds/pathpicker.h"
#include "../common/script/objcontroller.h"
#include "../studio/api.h"

#define SCRIPTPATHPICKER_PARENT PathPicker

// -----------------------------------------------------------------------
// Your wnd object class

class ScriptPathPicker: public SCRIPTPATHPICKER_PARENT {
  
  public:

    ScriptPathPicker();
    virtual ~ScriptPathPicker();

  private:
};

// -----------------------------------------------------------------------
class PathPickerScriptController: public ScriptObjectControllerI {
public:
  virtual const char *getClassName() { return "PathPicker"; }
  virtual const char *getAncestorClassName() { return "GuiObject"; }
  virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
  virtual int getNumFunctions();
  virtual const function_descriptor_struct *getExportedFunctions();
  virtual GUID getClassGuid() { return pathPickerGuid; }
  virtual ScriptObject *instantiate();
  virtual void destroy(ScriptObject *o);
  virtual void *encapsulate(ScriptObject *o);
  virtual void deencapsulate(void *o);

private:

  static function_descriptor_struct exportedFunction[];
  static scriptVar PathPicker_getPath(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar PathPicker_onPathChanged(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar newpath);
};

extern PathPickerScriptController *pathPickerController;


// -----------------------------------------------------------------------
// This defines the svc_xuiObject that exposes your wnd object

extern char PathPickerXuiObjectStr[];
extern char PathPickerXuiSvcName[];
class PathPickerXuiSvc : public XuiObjectSvc<ScriptPathPicker, PathPickerXuiObjectStr, PathPickerXuiSvcName> {};

#endif
