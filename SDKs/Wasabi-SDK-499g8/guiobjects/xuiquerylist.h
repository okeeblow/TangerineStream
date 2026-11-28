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
#ifndef __QUERYLIST_H
#define __QUERYLIST_H

#include "../common/guiobjwnd.h"
#include "../bfc/autobitmap.h"
#ifdef WASABI_COMPILE_METADB
#include "../bfc/wnds/autoquerylist.h"
#endif
#include "../common/script/objcontroller.h"

#ifdef WASABI_COMPILE_METADB
#define QUERYLIST_PARENT AutoQueryList
#else
#define QUERYLIST_PARENT GuiObjectWnd
#endif

// -----------------------------------------------------------------------
class ScriptQueryList : public QUERYLIST_PARENT {
  
  public:

    ScriptQueryList();
    virtual ~ScriptQueryList();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setTitle(const char *name);

#ifdef WASABI_COMPILE_METADB
    void onResetSubqueries();
    virtual int onAction(const char *action, const char *param=NULL, int x=-1, int y=-1, int p1=0, int p2=0, void *data=NULL, int datalen=0, RootWnd *source=NULL);
#endif

  private:

    enum {
      QUERYLIST_SETTITLE = 0,
    };
    int myxuihandle;
};

// -----------------------------------------------------------------------
class QueryListScriptController: public ScriptObjectControllerI {
  public:

    virtual const char *getClassName() { return "QueryList"; }
    virtual const char *getAncestorClassName() { return "GuiObject"; }
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(guiObjectGuid); }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid() { return queryListGuid; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

    static scriptVar querylist_onResetQuery(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

  private:

    static function_descriptor_struct exportedFunction[];
    
};

extern QueryListScriptController *queryListController;

// -----------------------------------------------------------------------
extern char QueryListXuiObjectStr[];
extern char QueryListXuiSvcName[];
class QueryListXuiSvc : public XuiObjectSvc<ScriptQueryList, QueryListXuiObjectStr, QueryListXuiSvcName> {};

#endif
