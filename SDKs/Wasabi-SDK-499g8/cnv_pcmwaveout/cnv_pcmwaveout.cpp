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
#include "pcmwaveout.h"
#include "cnv_pcmwaveout.h"

static WACNAME wac;
WAComponentClient *the = &wac;

#include "../studio/services/servicei.h"
static waServiceT<svc_mediaConverter, PcmWaveout> pcmwaveout;

// {D7827B8C-4697-405c-8B05-377538D5FABD}
static const GUID guid = 
{ 0xd7827b8c, 0x4697, 0x405c, { 0x8b, 0x5, 0x37, 0x75, 0x38, 0xd5, 0xfa, 0xbd } };

WACNAME::WACNAME() {
#ifdef FORTIFY
  FortifySetName("cnv_pcmwaveout.wac");
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
  api->service_register(&pcmwaveout);

  config.registerAttributes();
}

void WACNAME::onDestroy() {
  api->service_deregister(&pcmwaveout);
  WAComponentClient::onDestroy();
}
