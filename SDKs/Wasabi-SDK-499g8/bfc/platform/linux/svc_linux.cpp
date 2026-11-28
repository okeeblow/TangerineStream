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
#include "../../std.h"
#include "svc_linux.h"
#include "fontmaker_linux.h"

static WACNAME wac;
WACPARENT *the = &wac;                     

// {EF7E83B0-10ED-4be4-AFAF-D012F05C114E}
static const GUID guid =
{ 0xef7e83b0, 0x10ed, 0x4be4, { 0xaf, 0xaf, 0xd0, 0x12, 0xf0, 0x5c, 0x11, 0x4e } };


WACNAME::WACNAME() : WACPARENT("Linux Platform Service") {
  registerService(new FontMakerCreator<FontMaker_Linux>);
}

WACNAME::~WACNAME() {
}

GUID WACNAME::getGUID() {
  return guid;
}

void WACNAME::onCreate() {
}

void WACNAME::onDestroy() {
}

