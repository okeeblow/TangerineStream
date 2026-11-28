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
#include "cnv_rawpcm.h"
#include "rawpcm.h"

static WACNAME wac;
WAComponentClient *the = &wac;

#include "../studio/services/servicei.h"

// {683FA153-4055-467c-ABEE-5E35FA03C51E}
static const GUID guid = 
{ 0x683fa153, 0x4055, 0x467c, { 0xab, 0xee, 0x5e, 0x35, 0xfa, 0x3, 0xc5, 0x1e } };

_int nch("# of channels",2);
_int samplerate("Sample rate",44100);
_int bps("Bits per second",16);

WACNAME::WACNAME() : WAComponentClient("RAW files support") {
  registerService(new waServiceT<svc_mediaConverter, RawPcm>);
}

WACNAME::~WACNAME() {
}

GUID WACNAME::getGUID() {
  return guid;
}

void WACNAME::onRegisterServices() {
  api->core_registerExtension("*.raw","RAW Files");
  
  registerAttribute(&nch);
  registerAttribute(&samplerate);
  registerAttribute(&bps);
}
