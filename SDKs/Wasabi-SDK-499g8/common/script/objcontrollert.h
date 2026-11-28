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
#ifndef __SCRIPTOBJECTCONTROLLERT_H
#define __SCRIPTOBJECTCONTROLLERT_H

#include "objcontroller.h"

template <class T, class A>
class ScriptObjectControllerT : public ScriptObjectControllerI {
public:
  virtual const char *getClassName() { return T::scriptobject_getClassName(); }
  virtual const char *getAncestorClassName() { return A::scriptobject_getClassName(); }
  virtual ScriptObjectController *getAncestorController() {
    return WASABI_API_MAKI->maki_getController(A::scriptobject_getClassGuid());
  }
  virtual GUID getClassGuid() { return T::scriptobject_getClassGuid(); }
  virtual ScriptObject *instantiate() {
    return (new T)->getScriptObject();
  }
  virtual void destroy(ScriptObject *o) {
    T *t = static_cast<T *>(o->vcpu_getInterface(T::scriptobject_getClassGuid()));
    delete t;
  }
  virtual void *encapsulate(ScriptObject *o) { return NULL; }
  virtual void deencapsulate(void *o) { }

#if 0
  virtual int getNumFunctions() { return fn_descs.getNumItems(); }
  virtual const function_descriptor_struct *getExportedFunctions() {
    return fn_descs.enumRef(0);
  }
protected:
  void addFn(const char *fnname, scriptcb *cb) {
    function_descriptor_struct fds;
    fds.function_name = fnname;
    fds.nparams = cb->getNumParams();
    fds.physical_ptr = cb->
  }
private:
  TList fn_descs;
#endif
};

#if 0
class scriptcb {
public:
  virtual void *getNumParams()=0;
  virtual void *getfn()=0;
};

template <class T>
class scriptcb0v : public scriptcb {
  typedef void (T::*fnPtrType)();
public:
  scriptcb0v(fnPtrType _fn) : fn(_fn) { }
  static scriptVar call(SCRIPT_FUNCTION_PARAMS, ScriptObject *o) {
    SCRIPT_FUNCTION_INIT
    T *t = static_cast<T *>(o->vcpu_getInterface(T::scriptobject_getClassGuid()));
    if (t) (t->*fn)();
    RETURN_SCRIPT_VOID;
  }
  virtual void *getNumParams() { return 0; }
  virtual void *getfn() {
    return static_cast<void *>(call);
  }
};
#endif

#endif
