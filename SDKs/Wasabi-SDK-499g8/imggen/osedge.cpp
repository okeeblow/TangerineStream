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

#include "osedge.h"
#include "../studio/xmlreader.h"
#include "../bfc/util/pathparse.h"

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(OsEdgeGen_Svc);
DECLARE_SERVICETSINGLE(svc_imageGenerator, OsEdgeImage);
END_SERVICES(OsEdgeGen_Svc, _OsEdgeGen_Svc);

#ifdef _X86_
extern "C" { int _link_OsEdgeGen_Svc; }
#else
extern "C" { int __link_OsEdgeGen_Svc; }
#endif

#endif


int OsEdgeImage::testDesc(const char *desc) {
  return STRCASEEQL(desc, "$osedge");
}

ARGB32 *OsEdgeImage::genImage(const char *desc, int *has_alpha, int *w, int *h, XmlReaderParams *params) {
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

  RECT r = Std::makeRect(0, 0, _w, _h);

  BITMAPINFO bmi;
  ZERO(bmi);
  bmi.bmiHeader.biSize = sizeof(bmi);
  bmi.bmiHeader.biWidth = _w;
  bmi.bmiHeader.biHeight = -_h;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  // the rest are 0
  ARGB32 *bits;
  HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
  HDC hdc = CreateCompatibleDC(NULL);
  HBITMAP prevbmp = (HBITMAP)SelectObject(hdc, hbmp);

  unsigned long edgev = 0;
  if (STRCASEEQL(params->getItemValue("edge"), "bump")) edgev = EDGE_BUMP;
  else if (STRCASEEQL(params->getItemValue("edge"), "etched")) edgev = EDGE_ETCHED;
  else if (STRCASEEQL(params->getItemValue("edge"), "raised")) edgev = EDGE_RAISED;
  else if (STRCASEEQL(params->getItemValue("edge"), "sunken")) edgev = EDGE_SUNKEN;
  if (edgev == 0) edgev = EDGE_RAISED;

  unsigned long sides = 0;
  PathParser pp(params->getItemValue("sides"), ",");
  for (int i = 0; i < pp.getNumStrings(); i++) {
    char *p = pp.enumString(i);
    if (STRCASEEQL(p, "left")) sides |= BF_LEFT;
    if (STRCASEEQL(p, "top")) sides |= BF_TOP;
    if (STRCASEEQL(p, "right")) sides |= BF_RIGHT;
    if (STRCASEEQL(p, "bottom")) sides |= BF_BOTTOM;
    if (STRCASEEQL(p, "all")) sides |= BF_RECT;
    if (STRCASEEQL(p, "middle")) sides |= BF_MIDDLE;
    if (STRCASEEQL(p, "flat")) sides |= BF_FLAT;
    if (STRCASEEQL(p, "soft")) sides |= BF_SOFT;
    if (STRCASEEQL(p, "mono")) sides |= BF_MONO;
  }

// DO EET
  DrawEdge(hdc, &r, edgev, sides);

  MEMCPY(ret, bits, sizeof(ARGB32) * _w * _h);
  for (i = 0; i < _w * _h; i++) {	// force alpha
    ret[i] |= 0xff000000;
  }

  SelectObject(hdc, prevbmp);
  DeleteDC(hdc);
  DeleteObject(hbmp);

  *w = _w;
  *h = _h;
  *has_alpha = 1;	// will be optimized anyway

  return ret;
}
