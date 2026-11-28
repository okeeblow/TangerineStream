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

#include "ppmload.h"
#include "loader.h"
#include "../studio/services/servicei.h"

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(PpmLoader_Svc)
DECLARE_SERVICETSINGLE(svc_imageLoader, PpmLoad>);
END_SERVICES(PpmLoader_Svc, _PpmLoader_Svc);

#ifdef _X86_
extern "C" { int _link_PpmLoadSvc; }
#else
extern "C" { int __link_PpmLoadSvc; }
#endif

#else

static WACPpmLoad wac;
WAComponentClient *the = &wac;

// {70F911B2-FC5A-40c6-80C1-B082A9F18F80}
static const GUID guid = 
{ 0x70f911b2, 0xfc5a, 0x40c6, { 0x80, 0xc1, 0xb0, 0x82, 0xa9, 0xf1, 0x8f, 0x80 } };

WACPpmLoad::WACPpmLoad() {
  registerService(new waServiceTSingle<svc_imageLoader, PpmLoad>);
}

GUID WACPpmLoad::getGUID() {
  return guid;
}

#endif
