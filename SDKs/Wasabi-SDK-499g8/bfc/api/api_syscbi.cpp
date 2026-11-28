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
//<?#include "<class data="implementationheader"/>"
#include "api_syscbi.h"
//?>

#include "../../studio/api.h"
#include "../../studio/cbmgr.h"

api_syscb *sysCallbackApi = NULL;

int api_syscbI::syscb_registerCallback(SysCallback *cb, void *param) {
#ifdef WASABI_COMPILE_COMPONENTS
  CallbackManager::registerCallback(cb, param, WASABI_API_COMPONENT->getThisComponent());
#else
  CallbackManager::registerCallback(cb, param, NULL);
#endif
  return 1;
}

int api_syscbI::syscb_deregisterCallback(SysCallback *cb) {
#ifdef WASABI_COMPILE_COMPONENTS
  CallbackManager::deregisterCallback(cb, WASABI_API_COMPONENT->getThisComponent());
#else
  CallbackManager::deregisterCallback(cb, NULL);
#endif
  return 1;
}

int api_syscbI::syscb_issueCallback(int eventtype, int msg, int p1, int p2) {
  CallbackManager::issueCallback(eventtype, msg, p1, p2);
  return 1;
}

