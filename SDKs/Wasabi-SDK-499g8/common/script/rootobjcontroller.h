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
#ifndef __ROOTSCRIPTOBJECTCONTROLLER_IMPL_H
#define __ROOTSCRIPTOBJECTCONTROLLER_IMPL_H

/*<?<autoheader/>*/
class ScriptObject;
class ScriptObjectController;
/*?>*/

class RootScriptObjectController : public ScriptObjectControllerI {
  public:
    virtual const char *getClassName();
    virtual const char *getAncestorClassName();
    virtual ScriptObjectController *getAncestorController() { return NULL; }
    virtual int getNumFunctions();
    virtual const function_descriptor_struct *getExportedFunctions();
    virtual GUID getClassGuid();
    virtual ScriptObject *instantiate();
    virtual void destroy(ScriptObject *o);
    virtual void deencapsulate(void *o);
    virtual void *encapsulate(ScriptObject *o);
  private:
    static function_descriptor_struct exportedFunction[];
};

class RootObject_ScriptMethods {
  public:
    static scriptVar getClassName(SCRIPT_FUNCTION_PARAMS, ScriptObject *o); 
    static scriptVar onNotify(SCRIPT_FUNCTION_PARAMS, ScriptObject *on, scriptVar s, scriptVar s2, scriptVar i1, scriptVar i2); 
    static scriptVar notify(SCRIPT_FUNCTION_PARAMS, ScriptObject *on, scriptVar s, scriptVar s2, scriptVar i1, scriptVar i2); 
};

extern RootScriptObjectController *rootScriptObjectController;

#endif // __ROOTSCRIPTOBJECTCONTROLLER_IMPL_H
