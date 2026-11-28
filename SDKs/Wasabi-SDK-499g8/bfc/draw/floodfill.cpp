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

#include "floodfill.h"

FloodFill32::FloodFill32(ARGB32 *_bits, int _w, int _h, int _eightway) {
  bits = _bits;
  w = _w;
  ASSERT(w > 0);
  h = _h;
  ASSERT(h > 0);
  eightway = _eightway;
}

void FloodFill32::fill(int x, int y, ARGB32 _to_color) {
  ASSERT(x >= 0 && x < w);
  ASSERT(y >= 0 && y < h);
  from_color = bits[x + y * w];
  to_color = _to_color;

  pushPixel(x, y);

  while (!stack.isempty()) {
    POINT p;
    stack.pop(&p);
    x = p.x; y = p.y;
    ARGB32 *pixel = bits + y*w + x;
    if (testPixel(*pixel)) {
      *pixel = adjustPixel(*pixel);
      ASSERT(!testPixel(*pixel));
      // now scan all around
      for (int a = -1; a <= 1; a++)
        for (int b = -1; b <= 1; b++)
          if ( (eightway && (a || b)) || 
               ((a || b) && (!a || !b)) ) pushPixel(x+a, y+b);
    }
  }
}

void FloodFill32::pushPixel(int x, int y) {
  if (x < 0 || x >= w || y < 0 || y >= h) return;
  POINT p = { x, y };
  stack.push(p, w*h+1);
}

int FloodFill32::testPixel(ARGB32 color) {
  return (color == from_color);
}

ARGB32 FloodFill32::adjustPixel(ARGB32 pixel) {
  return to_color;
}

#define RED(a) (((a)>>16)&0xff)
#define GRN(a) (((a)>>8)&0xff)
#define BLU(a) (((a)&0xff))
#define ALP(a) (((a)>>24))

MagicWandFill::MagicWandFill(ARGB32 *bits, int w, int h, int eightway)
  : FloodFill32(bits, w, h, eightway), tolerance(0.2f) { }

int MagicWandFill::testPixel(ARGB32 color) {
  if (ALP(color) != 255) return 0;
  if (pixelCmp(color, from_color) <= tolerance) return 1;
  for (int i = 0; i < samples.getNumItems(); i++) {
    if (pixelCmp(color, samples.enumItem(i)) <= tolerance) return 1;
  }
  return 0;
}

ARGB32 MagicWandFill::adjustPixel(ARGB32 pixel) {
  return FloodFill32::adjustPixel(pixel);
}

void MagicWandFill::setTolerance(float t) {
  tolerance = MINMAX(t, 0.f, 1.f);
}

void MagicWandFill::addMagicPixel(int x, int y) {
  if (x < 0 || y < 0 || x >= w || y >= h) return;	// out of bounds
  ARGB32 color = bits[x + y * w];
  if (samples.haveItem(color)) return;
  samples.addItem(color);
//CUT DebugString("added color %x", color);
}

void MagicWandFill::addMagicPixelRegion(int x, int y, int w, int h) {
  for (int a = 0; a < w; a++)
    for (int b = 0; b < h; b++)
      addMagicPixel(x+a, y+h);
}

void MagicWandFill::clearMagicPixels() {
  samples.removeAll();
}

float MagicWandFill::pixelCmp(ARGB32 a, ARGB32 b) {
  return SQRT( SQR(RED(a) - RED(b))
    + SQR(GRN(a) - GRN(b))
    + SQR(BLU(a) - BLU(b)) ) / (441.f);
}
