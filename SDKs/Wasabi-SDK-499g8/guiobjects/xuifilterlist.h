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
#ifndef __FilterLIST_H
#define __FilterLIST_H

#include "../bfc/wnds/autofilterlist.h"
#include "../common/script/objcontroller.h"

#define FILTERLIST_PARENT AutoFilterList

// -----------------------------------------------------------------------
class ScriptFilterList : public FILTERLIST_PARENT {
public:
  ScriptFilterList();
  virtual ~ScriptFilterList();

  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

  void setXuiQueryList(const char *v);
  virtual int onInit();
  void setTitle(const char *name);
  void setAllString(const char *str);
  virtual const char *getAllString();
  virtual int onDeferredCallback(int p1, int p2);
  virtual int wantHScroll() { return 0; }

private:

  enum {
    FILTERLIST_SETFIELD = 0,
    FILTERLIST_SETQUERYLIST,
    FILTERLIST_SETTITLE,
    FILTERLIST_SETALLSTRING,
    FILTERLIST_SETORDER,
    FILTERLIST_SETLINKED,
    FILTERLIST_SETQUERYDIRECTION,
    FILTERLIST_FORCEVIRTUAL,
  };
  int myxuihandle;
  String querylist_id;
  String allstr;
  MultiQueryServer *serv;
};

// -----------------------------------------------------------------------
class FilterListScriptController: public ScriptObjectControllerI {
public:
  virtual const char *getClassName() { return "FilterList"; }
  virtual const char *getAncestorClassName() { return "GuiObject"; }
  virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
  virtual int getNumFunctions();
  virtual const function_descriptor_struct *getExportedFunctions();
  virtual GUID getClassGuid() { return filterListGuid; }
  virtual ScriptObject *instantiate();
  virtual void destroy(ScriptObject *o);
  virtual void *encapsulate(ScriptObject *o);
  virtual void deencapsulate(void *o);

private:

  static function_descriptor_struct exportedFunction[];
  static scriptVar Filterlist_setQueryList(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar id);
};

extern FilterListScriptController *filterListController;

// -----------------------------------------------------------------------
extern char FilterListXuiObjectStr[];
extern char FilterListXuiSvcName[];
class FilterListXuiSvc : public XuiObjectSvc<ScriptFilterList, FilterListXuiObjectStr, FilterListXuiSvcName> {};

#endif
