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

#include "xuigradientwnd.h"

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(XuiGradient_Svc);
DECLARE_SERVICE(XuiObjectCreator<GradientWndXuiSvc>);
END_SERVICES(XuiGradient_Svc, _XuiGradient_Svc);

#ifdef _X86_
extern "C" { int _link_GradientXuiSvc; }
#else
extern "C" { int __link_GradientXuiSvc; }
#endif

#endif


enum { P_X1, P_Y1, P_X2, P_Y2, P_POINTS, P_GG };

XuiGradientWnd::XuiGradientWnd() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "gradient_x1", P_X1, XUI_ATTRIBUTE_REQUIRED);
  addParam(myxuihandle, "gradient_y1", P_Y1, XUI_ATTRIBUTE_REQUIRED);
  addParam(myxuihandle, "gradient_x2", P_X2, XUI_ATTRIBUTE_REQUIRED);
  addParam(myxuihandle, "gradient_y2", P_Y2, XUI_ATTRIBUTE_REQUIRED);
  addParam(myxuihandle, "points", P_POINTS, XUI_ATTRIBUTE_REQUIRED);
  addParam(myxuihandle, "gammagroup", P_GG, XUI_ATTRIBUTE_IMPLIED);
}

int XuiGradientWnd::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return XUIGRADIENTWND_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);
  switch (xmlattributeid) {
    case P_X1: setX1((float)ATOF(value)); break;
    case P_Y1: setY1((float)ATOF(value)); break;
    case P_X2: setX2((float)ATOF(value)); break;
    case P_Y2: setY2((float)ATOF(value)); break;
    case P_POINTS: setPoints(value); break;
    case P_GG: setGammaGroup(value); break;
    default:
      return 0;
  }
  invalidate();	// what the hell
  return 1;
}
