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
//PORTABLE
#ifndef _GROUPLIST_H
#define _GROUPLIST_H

#include "script/guiobj.h"
#include "../common/guiobjwnd.h"
#include "../common/clickwnd.h"
#include "../bfc/ptrlist.h"

// {01E28CE1-B059-11d5-979F-E4DE6F51760A}
static const GUID grouplistGuid = 
{ 0x1e28ce1, 0xb059, 0x11d5, { 0x97, 0x9f, 0xe4, 0xde, 0x6f, 0x51, 0x76, 0xa } };

#define GROUPLIST_PARENT GuiObjectWnd

class GroupListScriptController: public GuiObjectScriptController {
  public:

    virtual const char *getClassName();
    virtual const char *getAncestorClassName();
    virtual ScriptObjectController *getAncestorController() { return guiController; }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid();
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  private:

    static function_descriptor_struct exportedFunction[];
    
};

extern GroupListScriptController *grouplistController;

class GroupList : public GROUPLIST_PARENT {
public:
  GroupList();
  virtual ~GroupList();

  Group *instantiate(const char *id, int n);
  void removeAll();
  Group *enumItem(int n);
  int getNumItems();
  void scrollToPercent(int p);
  void scrollTo(int y);
  void setRedraw(int i);
  void reposChildren();
  virtual int onResize();

  virtual int getPreferences(int what);

private:  

  void insert(const char *id, int where);

public:

  static scriptVar script_vcpu_instantiate(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar id, scriptVar n);
  static scriptVar script_vcpu_getNumItems(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar script_vcpu_enumItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar n);
  static scriptVar script_vcpu_removeAll(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
  static scriptVar script_vcpu_scrollToPercent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar n);
  static scriptVar script_vcpu_setRedraw(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar n);

  PtrList<Group> groups;
  int scrollY;
  int maxheight;
  int maxwidth;
  int redraw;
};

extern char groupListXuiObjectStr[];
extern char groupListXuiSvcName[];
class GroupListXuiSvc : public XuiObjectSvc<GroupList, groupListXuiObjectStr, groupListXuiSvcName> {};


#endif
