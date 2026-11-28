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
#ifndef __CFGSCRIPTOBJ_H
#define __CFGSCRIPTOBJ_H

#include "../common/script/rootobj.h"
#include "../common/script/objcontroller.h"
#include "../common/script/scriptobj.h"
#include <bfc/depview.h>
#include <studio/services/svc_scriptobj.h>
#include <bfc/attribs/attribs.h>

class CfgItem;
class ConfigObject;
class ConfigItemObject;
class ConfigAttributeObject;

extern ScriptObjectController *configController;
extern ScriptObjectController *configItemController;
extern ScriptObjectController *configAttributeController;

enum cfgtypes {
  CFG_INT = 0,
  CFG_BOOL = 1,
  CFG_FLOAT = 2,
  CFG_STRING = 3,
};

// -----------------------------------------------------------------------------------------------------
// ScriptObject Provider Service

class ConfigScriptObjectSvc : public svc_scriptObjectI {

public:
  ConfigScriptObjectSvc() {};
  virtual ~ConfigScriptObjectSvc() {};

  static const char *getServiceName() { return "Config Script Object Service"; }
  virtual ScriptObjectController *getController(int n);

  static void addItemObject(ConfigItemObject *item);
  static void addAttrObject(ConfigAttributeObject *attr);
  static void removeItemObject(ConfigItemObject *item);
  static void removeAttrObject(ConfigAttributeObject *attr);
};       

// -----------------------------------------------------------------------------------------------------
// Script classes GUIDS

// {593DBA22-D077-4976-B952-F4713655400B}
static const GUID CONFIG_SCRIPTOBJECT_GUID = 
{ 0x593dba22, 0xd077, 0x4976, { 0xb9, 0x52, 0xf4, 0x71, 0x36, 0x55, 0x40, 0xb } };

// {D4030282-3AAB-4d87-878D-12326FADFCD5}
static const GUID CONFIGITEM_SCRIPTOBJECT_GUID = 
{ 0xd4030282, 0x3aab, 0x4d87, { 0x87, 0x8d, 0x12, 0x32, 0x6f, 0xad, 0xfc, 0xd5 } };

// {24DEC283-B76E-4a36-8CCC-9E24C46B6C73}
static const GUID CONFIGATTRIBUTE_SCRIPTOBJECT_GUID = 
{ 0x24dec283, 0xb76e, 0x4a36, { 0x8c, 0xcc, 0x9e, 0x24, 0xc4, 0x6b, 0x6c, 0x73 } };

// -----------------------------------------------------------------------------------------------------
// ScriptObject Interfaces

//   Config 
class ConfigObject : public RootObjectInstance {
  
  public:

    ConfigObject();
    virtual ~ConfigObject();
    
    static scriptVar config_getItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar configitem_name);
    static scriptVar config_getItemByGuid(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar configitem_name);
    static scriptVar config_newItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar configitem_name, scriptVar guid);
  private:
    static ConfigItemObject *getItem(const char *nameorguid, ConfigObject *co);
    PtrList<ConfigItemObject> mylist;
    static PtrList<CfgItemI> ouraddeditems;
    static int numobjects;
};

//   ConfigItem
class ConfigItemObject : public RootObjectInstance {
  
  public:

    ConfigItemObject(CfgItem *item);
    virtual ~ConfigItemObject();

    ConfigAttributeObject *getAttribute(const char *name);
    const char *getGuid() { return guid; }
    ConfigAttributeObject *newAttribute(const char *name, const char *defval);

    CfgItem *getCfgItem() { return item; }

    static scriptVar configItem_getAttribute(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar attr_name);
    static scriptVar configItem_getGuid(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar configItem_newAttribute(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar attr_name, scriptVar defval);

  private:
    CfgItem *item;
    String guid;
    PtrList<ConfigAttributeObject> mylist;
};

//   ConfigAttribute
class ConfigAttributeObject : public RootObjectInstance, public DependentViewerTPtr<CfgItem> {
  
  public:

    ConfigAttributeObject(CfgItem *item, const char *attr, ConfigItemObject *parent);
    virtual ~ConfigAttributeObject();

    void setData(const char *data);
    const char *getData();
    ConfigItemObject *getParentItem() { return parent; }
    const char *getAttributeName() { return attr; }
    void setAutoDelete() { autodelete = 1; }

    virtual int viewer_onEvent(CfgItem *item, int event, int param, void *ptr, int ptrlen);
	
    static scriptVar configAttr_getData(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar configAttr_setData(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar val);
    static scriptVar configAttr_onDataChanged(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar configAttr_getParentItem(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar configAttr_getAttributeName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

  private:
    CfgItem *item;
    String attr;
    ConfigItemObject *parent;
    int autodelete;
};

// -----------------------------------------------------------------------------------------------------
// ScriptObjectControllers for our script classes

//   Config
class ConfigScriptObjectController : public ScriptObjectControllerI {
  public:
    virtual const char *getClassName() { return "Config"; }
    virtual const char *getAncestorClassName() { return "Object"; }
    virtual ScriptObjectController *getAncestorController() { return NULL; }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions() { return exportedFunction; }
    virtual GUID getClassGuid() { return CONFIG_SCRIPTOBJECT_GUID; }
    virtual int getInstantiable() { return 0; }
    virtual int getReferenceable() { return 0; }
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void *encapsulate(ScriptObject *o);
    virtual void deencapsulate(void *o);

  private:
    static function_descriptor_struct exportedFunction[];
};

//   ConfigItem
class ConfigItemScriptObjectController : public ScriptObjectControllerI {
  public:
    virtual const char *getClassName() { return "ConfigItem"; }
    virtual const char *getAncestorClassName() { return "Object"; }
    virtual ScriptObjectController *getAncestorController() { return NULL; }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions() { return exportedFunction; }
    virtual GUID getClassGuid() { return CONFIGITEM_SCRIPTOBJECT_GUID; }
    virtual int getInstantiable() { return 0; }
    virtual int getReferenceable() { return 1; }
    virtual ScriptObject *instantiate() { return NULL; };
    virtual void destroy(ScriptObject *o) { };
    virtual void *encapsulate(ScriptObject *o) { return NULL; };
    virtual void deencapsulate(void *o) {  };

  private:
    static function_descriptor_struct exportedFunction[];
};

//   ConfigAttribute
class ConfigAttributeScriptObjectController : public ScriptObjectControllerI {
  public:
    virtual const char *getClassName() { return "ConfigAttribute"; }
    virtual const char *getAncestorClassName() { return "Object"; }
    virtual ScriptObjectController *getAncestorController() { return NULL; }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions() { return exportedFunction; }
    virtual GUID getClassGuid() { return CONFIGATTRIBUTE_SCRIPTOBJECT_GUID; }
    virtual int getInstantiable() { return 0; }
    virtual int getReferenceable() { return 1; }
    virtual ScriptObject *instantiate() { return NULL; };
    virtual void destroy(ScriptObject *o) { };
    virtual void *encapsulate(ScriptObject *o) { return NULL; };
    virtual void deencapsulate(void *o) {  };

  private:
    static function_descriptor_struct exportedFunction[];
};

#endif

