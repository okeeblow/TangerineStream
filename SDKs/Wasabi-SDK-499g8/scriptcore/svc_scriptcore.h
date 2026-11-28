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
#ifndef __GUIVIDEOSVC_H
#define __GUIVIDEOSVC_H

#include "../studio/services/svc_scriptobj.h"

class CoreScriptObjectSvc : public svc_scriptObjectI {

public:
  CoreScriptObjectSvc() {};
  virtual ~CoreScriptObjectSvc() {};

  static const char *getServiceName() { return "Maki MediaCore Service"; }
  virtual ScriptObjectController *getController(int n);
}; 
      
class CoreAdminScriptObjectSvc : public svc_scriptObjectI {

public:
  CoreAdminScriptObjectSvc() {};
  virtual ~CoreAdminScriptObjectSvc() {};

  static const char *getServiceName() { return "Maki MediaCoreAdmin Service"; }
  virtual ScriptObjectController *getController(int n);
};       

#endif

