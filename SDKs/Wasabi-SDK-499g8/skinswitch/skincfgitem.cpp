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
#include "skincfgitem.h"

SkinCfgItem skinCfgItem;

// {EF2224B6-1EC6-4975-B910-50730898358B}
static const GUID skincfgitemguid = 
{ 0xef2224b6, 0x1ec6, 0x4975, { 0xb9, 0x10, 0x50, 0x73, 0x8, 0x98, 0x35, 0x8b } };

SkinCfgItem::SkinCfgItem() : CfgItemI("Skins", skincfgitemguid) {
  setCfgXml("config.skins");
}

SkinCfgItem::~SkinCfgItem() {
}
