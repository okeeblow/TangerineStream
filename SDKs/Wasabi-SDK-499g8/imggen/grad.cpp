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

#include "grad.h"

#include <studio/xmlreader.h>

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(GradientGen_Svc);
DECLARE_SERVICETSINGLE(svc_imageGenerator, GradientImage);
END_SERVICES(GradientGen_Svc, _GradientGen_Svc);

#ifdef _X86_
extern "C" { int _link_GradientGen_Svc; }
#else
extern "C" { int __link_GradientGen_Svc; }
#endif

#endif


int GradientImage::testDesc(const char *desc) {
  return STRCASEEQL(desc, "$gradient");
}

ARGB32 *GradientImage::genImage(const char *desc, int *has_alpha, int *w, int *h, XmlReaderParams *params) {
  int _w = ATOI(params->getItemValue("w"));
  if (_w == 0) _w = 1;
  int _h = ATOI(params->getItemValue("h"));
  if (_h == 0) _h = 1;
  if (_w <= 0 || _h <= 0) return NULL;

#ifdef WASABI_COMPILE_MEMMGR
  ARGB32 *ret = (ARGB32*)WASABI_API_MEMMGR->sysMalloc(_w * _h * sizeof(ARGB32));
#else
  ARGB32 *ret = (ARGB32*)MALLOC(_w * _h * sizeof(ARGB32));
#endif

  setX1((float)ATOF(params->getItemValue("gradient_x1")));
  setY1((float)ATOF(params->getItemValue("gradient_y1")));
  setX2((float)ATOF(params->getItemValue("gradient_x2")));
  setY2((float)ATOF(params->getItemValue("gradient_y2")));

  setPoints(params->getItemValue("points"));

  setMode(params->getItemValue("mode"));

  setReverseColors(TRUE);	// cuz we're imggen

  setAntialias(ATOI(params->getItemValue("antialias")));

  renderGradient(ret, _w, _h);

  *w = _w;
  *h = _h;
  *has_alpha = 1;	// will be optimized anyway

  return ret;
}
