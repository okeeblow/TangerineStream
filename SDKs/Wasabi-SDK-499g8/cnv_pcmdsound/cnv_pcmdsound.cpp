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
#include "pcmdsound.h"
#include "cnv_pcmdsound.h"

static WACNAME wac;
WAComponentClient *the = &wac;

#include "../studio/services/servicei.h"
static waServiceT<svc_mediaConverter, PcmDSound> pcmDSound;

// {14062313-85A6-4d9f-99C5-646148B7B415}
static const GUID guid = 
{ 0x14062313, 0x85a6, 0x4d9f, { 0x99, 0xc5, 0x64, 0x61, 0x48, 0xb7, 0xb4, 0x15 } };

WACNAME::WACNAME() : CfgItemI("DirectSound output") {
#ifdef FORTIFY
  FortifySetName("cnv_pcmdsound.wac");
  FortifyEnterScope();
#endif
}

WACNAME::~WACNAME() {
#ifdef FORTIFY
  FortifyLeaveScope();
#endif
}

GUID WACNAME::getGUID() {
  return guid;
}

void WACNAME::onRegisterServices() {
  api->service_register(&pcmDSound);
}

void WACNAME::onDestroy() {
  api->service_deregister(&pcmDSound);
  WAComponentClient::onDestroy();
}
