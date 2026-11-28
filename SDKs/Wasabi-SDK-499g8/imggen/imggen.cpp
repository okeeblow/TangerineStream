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
#include "imggen.h"

#include "solid.h"
#include "grad.h"
#include "osedge.h"
#include "poly.h"
#include "shadowwnd.h"

#include "../studio/services/servicei.h"

static WACNAME wac;
WAComponentClient *the = &wac;

// {9C9CB15E-2904-4df2-B8CE-FFBC6CD230DC}
static const GUID guid = 
{ 0x9c9cb15e, 0x2904, 0x4df2, { 0xb8, 0xce, 0xff, 0xbc, 0x6c, 0xd2, 0x30, 0xdc } };

WACNAME::WACNAME() {
  registerService(new waServiceFactoryTSingle<svc_imageGenerator, SolidImage>);
  registerService(new waServiceFactoryTSingle<svc_imageGenerator, GradientImage>);
  registerService(new waServiceFactoryTSingle<svc_imageGenerator, OsEdgeImage>);
  registerService(new waServiceFactoryTSingle<svc_imageGenerator, PolyImage>);
  registerService(new XuiObjectCreator<XuiShadowWndSvc>);
}

GUID WACNAME::getGUID() {
  return guid;
}
