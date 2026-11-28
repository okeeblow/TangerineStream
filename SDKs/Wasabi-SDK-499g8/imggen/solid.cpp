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

#include "solid.h"
#include "../studio/xmlreader.h"

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(SolidGen_Svc);
DECLARE_SERVICETSINGLE(svc_imageGenerator, SolidImage);
END_SERVICES(SolidGen_Svc, _SolidGen_Svc);

#ifdef _X86_
extern "C" { int _link_SolidGen_Svc; }
#else
extern "C" { int __link_SolidGen_Svc; }
#endif

#endif

int SolidImage::testDesc(const char *desc) {
  return STRCASEEQL(desc, "$solid");
}

void premultiply(ARGB32 *m_pBits, int nwords) {
  for (; nwords > 0; nwords--, m_pBits++) {
    unsigned char *pixel = (unsigned char *)m_pBits;
    unsigned int alpha = pixel[3];
    if (alpha == 255) continue;
    pixel[0] = (pixel[0] * alpha) >> 8;	// blue
    pixel[1] = (pixel[1] * alpha) >> 8;	// green
    pixel[2] = (pixel[2] * alpha) >> 8;	// red
  }
}

ARGB32 *SolidImage::genImage(const char *desc, int *has_alpha, int *w, int *h, XmlReaderParams *params) {
  int _w = ATOI(params->getItemValue("w"));
  if (_w == 0) _w = 1;
  int _h = ATOI(params->getItemValue("h"));
  if (_h == 0) _h = 1;
  if (_w <= 0 || _h <= 0) return NULL;
  ARGB32 color = BSWAP(WASABI_API_SKIN->parse(params->getItemValue("color"), "color")<<8);
  const char *alphastr = params->getItemValue("alpha");
  unsigned int alpha = (alphastr==NULL || *alphastr=='\0') ? 255 : ATOI(alphastr);
  color |= ((alpha & 0xff) << 24);

  premultiply(&color, 1);

#ifdef WASABI_COMPILE_MEMMGR
  ARGB32 *ret = (ARGB32*)WASABI_API_MEMMGR->sysMalloc(_w * _h * sizeof(ARGB32));
#else
  ARGB32 *ret = (ARGB32*)MALLOC(_w * _h * sizeof(ARGB32));
#endif

  MEMFILL<ARGB32>(ret, color, _w * _h);

  *w = _w;
  *h = _h;

  *has_alpha = (alpha == 255) ? 0 : 1;

  return ret;
}
