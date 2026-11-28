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

#include "config.h"

#include "resource.h"

#include "../studio/api.h"
#include "../bfc/svc_enum.h"
#include "../bfc/bitmap.h"

#include "cfgswitch.h"
#include "../bfc/wnds/skinwnd.h"

static WACConfig Config;
WAComponentClient *the = &Config;
WACConfig *configwac = &Config;

// {ED3CD0D9-5FE5-4c34-AFE9-52E714D4FAA4}
static const GUID guid = 
{ 0xed3cd0d9, 0x5fe5, 0x4c34, { 0xaf, 0xe9, 0x52, 0xe7, 0x14, 0xd4, 0xfa, 0xa4 } };

#include "../bfc/wndcreator.h"
#include "../studio/services/creators.h"
#include "../studio/services/servicei.h"

WACConfig::WACConfig() : WAComponentClient("Configurification Registry") {
  registerService(new WndCreateCreatorSingle< CreateWndByGuid<CfgSwitcher> >);
//  registerService(new WndCreateCreatorSingle< CreateBucketItem<CfgSwitcher> >);
  registerAutoPopup(CfgSwitcher::getWindowTypeGuid(), CfgSwitcher::getWindowTypeName());


  registerSkinFile("xml/config.xml");
}

GUID WACConfig::getGUID() {
  return guid;
}

