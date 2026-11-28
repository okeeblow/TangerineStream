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
#include "wa2skin.h"

#include "../studio/services/servicei.h"

#include "wa2xmlprovider.h"
#include "xuiwa2slider.h"
#include "xuiwa2eqslider.h"
#include "xuiwa2ms.h"
#include "xuiwa2time.h"
#include "../studio/services/creators.h"

static WACWa2Skin wac;
WAComponentClient *the = &wac;
WACWa2Skin *wa2skinwac = &wac;

// {07D986F2-0D16-47f6-8240-98A5834E0895}
static const GUID guid = 
{ 0x7d986f2, 0xd16, 0x47f6, { 0x82, 0x40, 0x98, 0xa5, 0x83, 0x4e, 0x8, 0x95 } };

WACWa2Skin::WACWa2Skin() {
  registerService(new waServiceT<svc_xmlProvider, Wa2XmlProvider>);
  registerService(new XuiObjectCreator<Wa2SliderXuiSvc>);
  registerService(new XuiObjectCreator<Wa2EqSliderXuiSvc>);
  registerService(new XuiObjectCreator<Wa2MsXuiSvc>);
  registerService(new XuiObjectCreator<Wa2TimeXuiSvc>);
}

GUID WACWa2Skin::getGUID() {
  return guid;
}