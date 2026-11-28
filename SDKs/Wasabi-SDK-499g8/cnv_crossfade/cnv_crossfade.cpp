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
#include "cnv_crossfade.h"
#include "crossfade.h"

#include <bfc/attribs/attribs.h>

static WACNAME wac;
WAComponentClient *the = &wac;

#include "../studio/services/servicei.h"

_int crossfade_time("Crossfade time", 5);	// in seconds*/
_bool crossfade_on_next("Crossfade on next/prev", true);

// {F1239F09-8CC6-4081-8519-C2AE99FCB14C}
static const GUID guid = 
{ 0xf1239f09, 0x8cc6, 0x4081, { 0x85, 0x19, 0xc2, 0xae, 0x99, 0xfc, 0xb1, 0x4c } };

WACNAME::WACNAME() : WAComponentClient("Crossfader") {
  registerService(new waServiceT<svc_mediaConverter, Crossfade>);
  registerAttribute(&crossfade_time);
  registerAttribute(&crossfade_on_next);
}

GUID WACNAME::getGUID() {
  return guid;
}
