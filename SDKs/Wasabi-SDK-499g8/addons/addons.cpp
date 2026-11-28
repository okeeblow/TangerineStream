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
#include "addons.h"
#include "xuiaddonslist.h"
#include "addonsmgr.h"
#include "../studio/services/creators.h"

static WACAddOns wac;
WACPARENT *the = &wac;
WACAddOns *addonswac = &wac;

// -----------------------------------------------------------------------
// {BA4C4602-88D7-4758-82F5-1E5E8975D760}
static const GUID guid = 
{ 0xba4c4602, 0x88d7, 0x4758, { 0x82, 0xf5, 0x1e, 0x5e, 0x89, 0x75, 0xd7, 0x60 } };

// {557D96B2-4C6A-43f3-BE6A-67EB3E198B14}
static const GUID PREF_ADDONS_GUID = 
{ 0x557d96b2, 0x4c6a, 0x43f3, { 0xbe, 0x6a, 0x67, 0xeb, 0x3e, 0x19, 0x8b, 0x14 } };

// -----------------------------------------------------------------------
WACAddOns::WACAddOns() {
  registerService(new XuiObjectCreator<AddOnsListXuiSvc>);
  registerCfgItem(&addonsmgr);
  registerSkinFile("xml/addons.xml");
}

// -----------------------------------------------------------------------
WACAddOns::~WACAddOns() {
}

// -----------------------------------------------------------------------
const char *WACAddOns::getName() {
  return "Add-Ons Manager";
}

// -----------------------------------------------------------------------
GUID WACAddOns::getGUID() {
  return guid;
}

// -----------------------------------------------------------------------
void WACAddOns::onCreate() {
  api->syscb_registerCallback(&addonsmgr);
  api->preferences_registerGroup("wasabi.config.addons", "Add-Ons/SkinMods", PREF_ADDONS_GUID);
}

// -----------------------------------------------------------------------
void WACAddOns::onDestroy() {
  WACPARENT::onDestroy();
  api->syscb_deregisterCallback(&addonsmgr);
}

