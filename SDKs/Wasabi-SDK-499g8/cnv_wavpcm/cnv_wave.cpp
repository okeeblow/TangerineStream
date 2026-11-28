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
#include "cnv_wave.h"
#include "wave.h"

static WACNAME wac;
WAComponentClient *the = &wac;

#include "../studio/services/servicei.h"
static waServiceT<svc_mediaConverter, Wave> wave;

// {98C54B21-75E3-411c-BD33-2F1307D3E141}
static const GUID guid = 
{ 0x98c54b21, 0x75e3, 0x411c, { 0xbd, 0x33, 0x2f, 0x13, 0x7, 0xd3, 0xe1, 0x41 } };
/*
_int nch;
_int samplerate;
_int bps;
*/
WACNAME::WACNAME()  {
#ifdef FORTIFY
  FortifySetName("cnv_wave.wac");
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
  api->service_register(&wave);
  api->core_registerExtension("*.wav","WAV Files","Audio");
  api->core_registerExtension("*.aiff;*.aif","AIFF Files","Audio");
  api->core_registerExtension("*.au;*.snd","AU/SND Files","Audio");
  api->core_registerExtension("*.voc","VOC Files","Audio");
  //api->core_registerExtension("*.iff","IFF Files","Audio");
  //api->core_registerExtension("*.svx","8SVX Files","Audio");
  //api->core_registerExtension("*.aud","AUD Files","Audio");


/*  
  nch.setName("# of channels");
  nch=2;
  registerAttribute(&nch);
  samplerate.setName("Sample rate");
  samplerate=44100;
  registerAttribute(&samplerate);
  bps.setName("Bits per second");
  bps=16;
  registerAttribute(&bps);*/
}

void WACNAME::onDestroy() {
  api->service_deregister(&wave);
  WAComponentClient::onDestroy();
}
