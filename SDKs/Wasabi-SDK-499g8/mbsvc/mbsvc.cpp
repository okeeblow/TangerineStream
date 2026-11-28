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
#include "mbsvc.h"
#include "svc.h"
#include "../studio/genwnd.h"

static WACNAME wac;
WAComponentClient *the = &wac;
WACNAME *wacmb = &wac;

#include "../studio/services/servicei.h"

// {181BE599-2249-4a1c-8283-4EE85FE8EC86}
static const GUID guid = 
{ 0x181be599, 0x2249, 0x4a1c, { 0x82, 0x83, 0x4e, 0xe8, 0x5f, 0xe8, 0xec, 0x86 } };

WACNAME::WACNAME() {
  registerService(new waServiceFactoryT<svc_miniBrowser, MbSvc>);
}

WACNAME::~WACNAME() {
}

GUID WACNAME::getGUID() {
  return guid;
}

