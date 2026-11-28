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

#include "poly.h"

#include "../studio/xmlreader.h"

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(PolyGen_Svc);
DECLARE_SERVICETSINGLE(svc_imageGenerator, PolyImage);
END_SERVICES(PolyGen_Svc, _PolyGen_Svc);

#ifdef _X86_
extern "C" { int _link_PolyGen_Svc; }
#else
extern "C" { int __link_PolyGen_Svc; }
#endif

#endif

int PolyImage::testDesc(const char *desc) {
  return STRCASEEQL(desc, "$polygon");
}

void premultiply(ARGB32 *m_pBits, int nwords);

#include "../bfc/draw/drawpoly.h"

ARGB32 *PolyImage::genImage(const char *desc, int *has_alpha, int *w, int *h, XmlReaderParams *params) {
  int _w = ATOI(params->getItemValue("w"));
  if (_w == 0) _w = 1;
  int _h = ATOI(params->getItemValue("h"));
  if (_h == 0) _h = 1;
  if (_w <= 0 || _h <= 0) return NULL;

  const char *bgcolorstr = params->getItemValue("bgcolor");
  ARGB32 bgcolor = (bgcolorstr == NULL || *bgcolorstr=='\0') ? 0 : BSWAP(WASABI_API_SKIN->parse(params->getItemValue("bgcolor"), "color")<<8);
  const char *bgalphastr = params->getItemValue("bgalpha");
  unsigned int bgalpha = (bgalphastr==NULL || *bgalphastr=='\0') ? 0 : ATOI(bgalphastr);
  bgcolor |= ((bgalpha & 0xff) << 24);

  premultiply(&bgcolor, 1);

#ifdef WASABI_COMPILE_MEMMGR
  ARGB32 *ret = (ARGB32*)WASABI_API_MEMMGR->sysMalloc(_w * _h * sizeof(ARGB32));
#else
  ARGB32 *ret = (ARGB32*)MALLOC(_w * _h * sizeof(ARGB32));
#endif

  MEMFILL<ARGB32>(ret, bgcolor, _w * _h);

  Draw::drawPointList(ret, _w, _h, params->getItemValue("points"));

  *w = _w;
  *h = _h;
  *has_alpha = 1;
  return ret;
}
