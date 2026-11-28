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
#ifndef __SCRIPTAPI_H
#define __SCRIPTAPI_H

#include "../../bfc/api/api_makii.h"

class ScriptApi : public api_makiI {
  public:
    ScriptApi();
    virtual ~ScriptApi();

    virtual void maki_pushObject(void *o);
    virtual void maki_pushInt(int v);
    virtual void maki_pushBoolean(int b);
    virtual void maki_pushFloat(float f);
    virtual void maki_pushDouble(double d);
    virtual void maki_pushString(const char *s);
    virtual void maki_pushVoid();
    virtual void maki_pushAny(scriptVar v);
    virtual void *maki_popObject();
    virtual int maki_popInt();
    virtual int maki_popBoolean();
    virtual float maki_popFloat();
    virtual double maki_popDouble();
    virtual const char *maki_popString();
    virtual scriptVar maki_popAny();
    virtual void maki_popDiscard();
    virtual const char *maki_getFunction(int dlfid, int *n, ScriptObjectController **p);
    virtual int maki_addDlfRef(ScriptObjectController *o, const char *function_name, void *host);
    virtual void maki_addDlfClassRef(ScriptObjectController *o, void *host);
    virtual void maki_remDlfRef(void *host);
    virtual scriptVar maki_callFunction(ScriptObject *o, int dlfid, scriptVar **params);
    virtual scriptVar maki_triggerEvent(ScriptObject *o, int dlfid, int np, int scriptid0=-1);
    virtual int maki_getScriptInt(scriptVar v);
    virtual int maki_getScriptBoolean(scriptVar v);
    virtual float maki_getScriptFloat(scriptVar v);
    virtual double maki_getScriptDouble(scriptVar v);
    virtual const char *maki_getScriptString(scriptVar v);
    virtual ScriptObject *maki_getScriptObject(scriptVar v);
    virtual scriptVar maki_updateDlf(maki_cmd *cmd, int *dlfid, int *linkcount);
    virtual ScriptObject *maki_instantiate(GUID classguid);
    virtual void maki_destroy(ScriptObject *o);
    virtual void *maki_encapsulate(GUID classguid, ScriptObject *o);
    virtual void maki_deencapsulate(GUID classguid, void *o);
    virtual ScriptObjectController *maki_getController(GUID scriptclass);
    virtual int maki_createOrphan(int type);
    virtual void maki_killOrphan(int id);
    virtual void maki_setObjectAtom(const char *atomname, ScriptObject *object);
    virtual ScriptObject *maki_getObjectAtom(const char *atomname);
#ifdef WASABI_COMPILE_WND
    virtual ScriptObject *maki_findObject(const char *name);
#endif
    virtual void vcpu_addScriptObject(ScriptObject *o);
    virtual void vcpu_removeScriptObject(ScriptObject *o);
    virtual int vcpu_getCacheCount();
    virtual int vcpu_isValidScriptId(int id);
    virtual int vcpu_mapVarId(int varid, int scriptid);
    virtual int vcpu_getUserAncestorId(int varid, int scriptid);
    virtual int vcpu_getNumEvents();
    virtual int vcpu_getEvent(int event, int *dlf, int *script, int *var);
    virtual int vcpu_getComplete();
    virtual const char *vcpu_getClassName(int vcpuid, int localclassid);
};

extern api_maki *makiApi;

#endif
