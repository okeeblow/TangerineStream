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
#include "cnv_vorbis.h"
#include "vorbis.h"
#include "vorbis_nsv.h"

static WACNAME wac;
WAComponentClient *the = &wac;

// {6EBEA3E7-9C4B-4ed6-942C-0ECAB9645A47}
static const GUID guid = 
{ 0x6ebea3e7, 0x9c4b, 0x4ed6, { 0x94, 0x2c, 0xe, 0xca, 0xb9, 0x64, 0x5a, 0x47 } };


WACNAME::WACNAME()  {
#ifdef FORTIFY
  FortifySetName("cnv_vorbis.wac");
  FortifyEnterScope();
#endif
  registerService(new waServiceT<svc_mediaConverter, VorbisDec>);
  registerService(new waServiceT<svc_mediaConverter, VorbisDecNsv>);
}

WACNAME::~WACNAME() {
#ifdef FORTIFY
  FortifyLeaveScope();
#endif
}

GUID WACNAME::getGUID() {
  return guid;
}

void WACNAME::onCreate() {
  api->core_registerExtension("*.ogg","Ogg Vorbis files","Audio");
  registerSkinFile("Wacs/xml/vorbis/info.xml"); 

}

void WACNAME::onDestroy() {
	DeferredUpdate::Flush();
	DeferredUpdate::list.freeAll();  
}
