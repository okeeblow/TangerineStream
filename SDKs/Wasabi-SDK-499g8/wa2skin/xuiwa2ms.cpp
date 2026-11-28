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
#include "xuiwa2ms.h"
#include "../bfc/bitmap.h"
#include "../bfc/canvas.h"
#include "../studio/api.h"

char Wa2MsXuiObjectStr[] = "Wa2Ms"; // This is the xml tag
char Wa2MsXuiSvcName[] = "Wa2Ms XuiObject Service";

Wa2Ms::Wa2Ms() {
  inactiveImage=activeImage=NULL;
  isactive=0;

  xuihandle=newXuiHandle();
  addParam(xuihandle, "inactive", WA2MS_INACTIVE, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "active", WA2MS_ACTIVE, XUI_ATTRIBUTE_IMPLIED);
  addParam(xuihandle, "string", WA2MS_STRING, XUI_ATTRIBUTE_IMPLIED);
}

Wa2Ms::~Wa2Ms() {
  delete(inactiveImage);
  delete(activeImage);
  api->core_delCallback(0, this);
}

int Wa2Ms::setXuiParam(int _xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle == _xuihandle) {
    switch(xmlattributeid) {
      case WA2MS_INACTIVE: inactivename=value; return 1;
      case WA2MS_ACTIVE: activename=value; return 1;
      case WA2MS_STRING: str=value; return 1;
    }
  }
  return WA2MS_PARENT::setXuiParam(_xuihandle,xmlattributeid,xmlattributename,value);
}

int Wa2Ms::onInit() {
  WA2MS_PARENT::onInit();

  if(!inactivename.isempty()) {
    inactiveImage=new SkinBitmap(inactivename);
  }
  if(!activename.isempty()) {
    activeImage=new SkinBitmap(activename);
  }
  api->core_addCallback(0, this);
  return 0;
}

int Wa2Ms::onPaint(Canvas *canvas) {
  RECT r,r2;
  r.left=0;
  r.top=0;
  getClientRect(&r2);
  if(!isactive && inactiveImage) {
    r.right=inactiveImage->getWidth();
    r.bottom=inactiveImage->getHeight();
    inactiveImage->blitToRect(canvas,&r,&r2);
  } else if(isactive && activeImage) {
    r.right=activeImage->getWidth();
    r.bottom=activeImage->getHeight();
    activeImage->blitToRect(canvas,&r,&r2);
  }
  return 0;
}

int Wa2Ms::corecb_onInfoChange(const char *info) {
  int n=0;
  if(!str.isempty() && STRSTR(info,str)) n=1;
  if(n!=isactive) {
    isactive=n;
    invalidate();
  }
  return 0;
}

int Wa2Ms::corecb_onStopped() {
  if(isactive) {
    isactive=0;
    invalidate();
  }
  return 0;
}
