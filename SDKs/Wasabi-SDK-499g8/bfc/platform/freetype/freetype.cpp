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
#include "../../std.h"
#include "freetype.h"

#include "../freetype/freetypefont.h"

#ifndef _WASABIRUNTIME

BEGIN_SYS_SERVICES(FreeTypeFontRenderer_Svc)
DECLARE_SERVICE(FontCreator<FreeTypeFont>);
END_SERVICES(FreeTypeFontRenderer_Svc, _FreeTypeFontRenderer_Svc);

#ifdef _X86_
extern "C" { int _link_FreeTypeFontRenderer_Svc; }
#else
extern "C" { int __link_FreeTypeFontRenderer_Svc; }
#endif

#else

static WACNAME wac;
WACPARENT *the = &wac;                     

// {EF7E83B0-10ED-4be4-AFAF-D012F05C114E}
static const GUID guid =
{ 0xef7e83b0, 0x10ed, 0x4be4, { 0xaf, 0xaf, 0xd0, 0x12, 0xf0, 0x5c, 0x11, 0x4e } };


WACNAME::WACNAME() : WACPARENT("Freetype Font Service") {
  registerService(new FontCreator<FreeTypeFont>);
}

WACNAME::~WACNAME() {
}

GUID WACNAME::getGUID() {
  return guid;
}

void WACNAME::onRegisterServices() {
#ifdef WASABI_COMPILE_CONFIG // if not, you need to set the font renderer by hand
  // Here is where we hijack the default renderer and do it all freetype style.
  const GUID options_guid = 
  { 0x280876cf, 0x48c0, 0x40bc, { 0x8e, 0x86, 0x73, 0xce, 0x6b, 0xb4, 0x62, 0xe5 } };
  CfgItem *options = WASABI_API_CONFIG->config_getCfgItemByGuid(options_guid);
  if (options) {
    const char *attr = "Font Renderer";
    const char *name = "Freetype";
    options->setData(attr, name);
  }  
  // the fonts system uses this attrib for what font system to try to use.
#endif
}

void WACNAME::onCreate() {
}

void WACNAME::onDestroy() {
}

#endif
