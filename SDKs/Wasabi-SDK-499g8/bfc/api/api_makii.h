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
#ifndef _API_MAKII_H
#define _API_MAKII_H

/*[interface.header.h]
#include "common/script/scriptvar.h"
#include "common/script/vcputypes.h"
#include "common/script/objcontroller.h"
*/

#include "api_maki.h"
#include "api_makix.h"

class ScriptObject;
class ScriptObjectController;

class api_makiI : public api_makiX {
  public:
  api_makiI();
  virtual ~api_makiI();
  DISPATCH(10) virtual void maki_pushObject(void *o);
  DISPATCH(20) virtual void maki_pushInt(int i);
  DISPATCH(30) virtual void maki_pushBoolean(int b);
  DISPATCH(40) virtual void maki_pushFloat(float f);
  DISPATCH(50) virtual void maki_pushDouble(double d);
  DISPATCH(60) virtual void maki_pushString(const char *s);
  DISPATCH(70) virtual void maki_pushVoid();
  DISPATCH(80) virtual void maki_pushAny(scriptVar v);
  DISPATCH(90) virtual void *maki_popObject();
  DISPATCH(100) virtual int maki_popInt();
  DISPATCH(110) virtual int maki_popBoolean();
  DISPATCH(120) virtual float maki_popFloat();
  DISPATCH(130) virtual double maki_popDouble();
  DISPATCH(140) virtual const char *maki_popString();
  DISPATCH(150) virtual scriptVar maki_popAny();
  DISPATCH(160) virtual void maki_popDiscard();
  DISPATCH(170) virtual const char *maki_getFunction(int dlfid, int *nparams, ScriptObjectController **p);
  DISPATCH(180) virtual int maki_addDlfRef(ScriptObjectController *o, const char *function_name, void *host);
  DISPATCH(190) virtual void maki_addDlfClassRef(ScriptObjectController *o, void *host);
  DISPATCH(200) virtual void maki_remDlfRef(void *host);
  DISPATCH(210) virtual scriptVar maki_callFunction(ScriptObject *o, int dlfid, scriptVar **params);
  DISPATCH(220) virtual scriptVar maki_triggerEvent(ScriptObject *o, int dlfid, int np, int scriptid = -1);
  DISPATCH(230) virtual int maki_getScriptInt(scriptVar v);
  DISPATCH(240) virtual int maki_getScriptBoolean(scriptVar v);
  DISPATCH(250) virtual float maki_getScriptFloat(scriptVar v);
  DISPATCH(260) virtual double maki_getScriptDouble(scriptVar v);
  DISPATCH(270) virtual const char *maki_getScriptString(scriptVar v);
  DISPATCH(280) virtual ScriptObject *maki_getScriptObject(scriptVar v);
  DISPATCH(290) virtual scriptVar maki_updateDlf(maki_cmd *cmd, int *dlfid, int *linkcount);
  DISPATCH(300) virtual ScriptObject *maki_instantiate(GUID classguid);
  DISPATCH(310) virtual void maki_destroy(ScriptObject *o);
  DISPATCH(320) virtual void *maki_encapsulate(GUID classguid, ScriptObject *o);
  DISPATCH(330) virtual void maki_deencapsulate(GUID classguid, void *o);
  DISPATCH(340) virtual ScriptObjectController *maki_getController(GUID scriptclass);
  DISPATCH(350) virtual int maki_createOrphan(int type);
  DISPATCH(360) virtual void maki_killOrphan(int id);
  DISPATCH(370) virtual void maki_setObjectAtom(const char *atomname, ScriptObject *object);
  DISPATCH(380) virtual ScriptObject *maki_getObjectAtom(const char *atomname);
/*[interface.maki_findObject.cpp]#ifdef WASABI_COMPILE_WND*/
/*[interface.maki_findObject.h]#ifdef WASABI_COMPILE_WND*/
/*[dispatchable.maki_findObject.enum]#ifdef WASABI_COMPILE_WND*/
/*[dispatchable.maki_findObject.bridge]#ifdef WASABI_COMPILE_WND*/
#ifdef WASABI_COMPILE_WND
  DISPATCH(390) virtual ScriptObject *maki_findObject(const char *name);
#endif
/*[interface.vcpu_addScriptObject.cpp]#endif*/
/*[interface.vcpu_addScriptObject.h]#endif*/
/*[dispatchable.vcpu_addScriptObject.enum]#endif*/
/*[dispatchable.vcpu_addScriptObject.bridge]#endif*/
  DISPATCH(400) virtual void vcpu_addScriptObject(ScriptObject *o);
  DISPATCH(410) virtual void vcpu_removeScriptObject(ScriptObject *o);
  DISPATCH(420) virtual int vcpu_getCacheCount();
  DISPATCH(430) virtual int vcpu_isValidScriptId(int id);
  DISPATCH(440) virtual int vcpu_mapVarId(int varid, int scriptid);
  DISPATCH(450) virtual int vcpu_getUserAncestorId(int varid, int scriptid);
  DISPATCH(460) virtual int vcpu_getNumEvents();
  DISPATCH(470) virtual int vcpu_getEvent(int event, int *dlf, int *script, int *var);
  DISPATCH(480) virtual int vcpu_getComplete();
  DISPATCH(490) virtual const char *vcpu_getClassName(int vcpuid, int localclassid);

  NODISPATCH void init();
};

/*[interface.footer.h]
// {F2398F09-63B0-4442-86C9-F8BC473F6DA7}
static const GUID makiApiServiceGuid = 
{ 0xf2398f09, 0x63b0, 0x4442, { 0x86, 0xc9, 0xf8, 0xbc, 0x47, 0x3f, 0x6d, 0xa7 } };

extern api_maki *makiApi;
*/

#endif
