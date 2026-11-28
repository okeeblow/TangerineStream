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

#include "skinswitch.h"
#include "resource.h"
#include "../studio/services/servicei.h"
#include "../studio/services/creators.h"

#include "../studio/api.h"
#include "skincfgitem.h"
#include "xuiskinlist.h"
#include "xuiskinshot.h"

// {641C4672-CF79-4444-BC46-8802B14D693C}
static const GUID PREF_SKINSWITCH_GUID = 
{ 0x641c4672, 0xcf79, 0x4444, { 0xbc, 0x46, 0x88, 0x2, 0xb1, 0x4d, 0x69, 0x3c } };


static WACskinswitch skinswitch;
WAComponentClient *the = &skinswitch;

// {CC5539D5-2087-481c-94E8-6F4DA80FC49D}
static const GUID guid = 
{ 0xcc5539d5, 0x2087, 0x481c, { 0x94, 0xe8, 0x6f, 0x4d, 0xa8, 0xf, 0xc4, 0x9d } };

WACskinswitch::WACskinswitch() {
  registerService(new XuiObjectCreator<SkinListXuiSvc>);
  registerService(new XuiObjectCreator<SkinShotXuiSvc>);
  registerSkinFile("xml/skins.xml"); 
  registerCfgItem(&skinCfgItem);
  registerPrefGroup("config.skins", "Skins", PREF_SKINSWITCH_GUID);
}

WACskinswitch::~WACskinswitch() {
}

GUID WACskinswitch::getGUID() {
  return guid;
}

void WACskinswitch::onCreate() {
}
