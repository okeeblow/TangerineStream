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
#include "precomp.h"
#include "scriptcore.h"
#include "coreobj.h"
#include "svc_scriptcore.h"

#include "resource.h"
#include "../studio/api.h"

static WACScriptCore scriptcore;
WAComponentClient *the = &scriptcore;
WACScriptCore *scriptcorewac = &scriptcore;

// {50E3B860-7070-4835-A7BA-3FBE1DCFB5F6}
static const GUID guid = 
{ 0x50e3b860, 0x7070, 0x4835, { 0xa7, 0xba, 0x3f, 0xbe, 0x1d, 0xcf, 0xb5, 0xf6 } };

#include "../studio/services/creators.h"	

WACScriptCore::WACScriptCore() {
  registerService(new ScriptObjectCreator<CoreScriptObjectSvc>);
  registerService(new ScriptObjectCreator<CoreAdminScriptObjectSvc>);}

GUID WACScriptCore::getGUID() {
  return guid;
}

void WACScriptCore::onCreate() {
}

void WACScriptCore::onDestroy() {
// be sure to delete all your windows etc HERE, not in the destructor
// because the API pointer might be invalid in the destructor
}

