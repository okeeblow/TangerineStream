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

#include "loader.h"

int PpmLoad::isMine(const char *filename) {
  return STRCASEEQLSAFE(Std::extension(filename), "ppm");
}

int PpmLoad::getHeaderSize() {
  return 2;
}

int PpmLoad::testData(const void *data, int datalen) {
  if (datalen < getHeaderSize()) return 0;
  return !MEMCMP(data, "P6", 2) && ISSPACE(((const char *)data)[2]);
}

ARGB32 *PpmLoad::loadImage(const void *data, int datalen, int *w, int *h, XmlReaderParams *params) {
  if (!testData(data, datalen)) return NULL;
  const char *p = ((const char *)data)+2;	// skip P6
  const char *bounds = (const char *)data + datalen;
// skip whitespace
  while (ISSPACE(*p)) { p++; if (p >= bounds) return NULL; }
  if (*p == '#') {
// skip fuckin comment
    p++;
    while (*p != '\r' && *p != '\n') { p++; if (p >= bounds) return NULL; }
// skip whitespace
    while (ISSPACE(*p)) { p++; if (p >= bounds) return NULL; }
  }
// get width
  int _w = ATOI(p);
  if (_w <= 0) return NULL;
  while (!ISSPACE(*p)) { p++; if (p >= bounds) return NULL; }
// skip whitespace
  p++; while (ISSPACE(*p)) { p++; if (p >= bounds) return NULL; }
// get height
  int _h = ATOI(p);
  if (_h <= 0) return NULL;
  while (!ISSPACE(*p)) { p++; if (p >= bounds) return NULL; }
// skip whitespace
  p++; while (ISSPACE(*p)) { p++; if (p >= bounds) return NULL; }
// get maxval
  int mv = ATOI(p);
  if (mv <= 0 || mv > 255) return NULL;
  while (!ISSPACE(*p)) { p++; if (p >= bounds) return NULL; }
// skip single whitespace
  p++; if (p >= bounds) return NULL;

  int amt_to_copy = ((const char *)data + datalen) - p;
  if (amt_to_copy > _w * _h * 3) return NULL;
  
  ARGB32 *ret = (ARGB32*)WASABI_API_MEMMGR->sysMalloc(_w * _h * sizeof(ARGB32));
  const unsigned char *src = (const unsigned char *)p;
  ARGB32 *dest = ret;
  for (int i = 0; i < _w*_h; i++) {
    unsigned int red = src[0], grn = src[1], blu = src[2];
    if (mv != 255) {
      float vv = (float)mv / 255.f;
      red = (int)floor(((float)red / vv)); ASSERT(red <= 255);
      grn = (int)floor(((float)grn / vv)); ASSERT(grn <= 255);
      blu = (int)floor(((float)blu / vv)); ASSERT(blu <= 255);
    }
    *dest++ = 0xff000000 | (red << 16) | (grn << 8) | blu;
    src += 3;
  }

  premultiplyARGB32(ret, _w * _h);

  *w = _w;
  *h = _h;

  return ret;
}
