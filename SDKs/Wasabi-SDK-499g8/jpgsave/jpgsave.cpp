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

#include "jpgsave.h"
#include <studio/services/servicei.h>
#include "saver.h"

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(JpgLoader_Svc)
DECLARE_SERVICETSINGLE(svc_imageLoader, JpgLoad);
END_SERVICES(JpgLoader_Svc, _JpgLoader_Svc);

#ifdef _X86_
extern "C" { int _link_JpgLoadSvc; }
#else
extern "C" { int __link_JpgLoadSvc; }
#endif

#else

static WACNAME wac;
WAComponentClient *the = &wac;

// {243389F2-357B-49aa-A6C8-EA5AC74EB4DB}
static const GUID guid = 
{ 0x243389f2, 0x357b, 0x49aa, { 0xa6, 0xc8, 0xea, 0x5a, 0xc7, 0x4e, 0xb4, 0xdb } };

WACNAME::WACNAME() {
  registerService(new waServiceTSingle<svc_imageWriter, JpgSave>);
}

WACNAME::~WACNAME() {
}

GUID WACNAME::getGUID() {
  return guid;
}

#endif
