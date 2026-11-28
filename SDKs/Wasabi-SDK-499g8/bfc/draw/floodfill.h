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
#ifndef _FLOODFILL_H
#define _FLOODFILL_H

#include <bfc/std.h>
#include <bfc/stack.h>

class FloodFill32 {
public:
  FloodFill32(ARGB32 *bits, int w, int h, int eightway=TRUE);

  void fill(int x, int y, ARGB32 to_color=0);

protected:
  // you can override these if you want to ... but make sure your
  // adjustPixel changes the pixel enough so that testPixel returns FALSE!
  virtual int testPixel(ARGB32 color);
  virtual ARGB32 adjustPixel(ARGB32 pixel);

  ARGB32 from_color, to_color;

protected:
  ARGB32 *bits;
  int w, h;

private:
  void pushPixel(int x, int y);

  Stack<POINT> stack;
  int eightway;
};

class MagicWandFill : public FloodFill32 {
public:
  MagicWandFill(ARGB32 *bits, int w, int h, int eightway=TRUE);

  virtual int testPixel(ARGB32 color);
  virtual ARGB32 adjustPixel(ARGB32 pixel);

  void setTolerance(float t);
  void addMagicPixel(int x, int y);
  void addMagicPixelRegion(int x, int y, int w, int h);
  void clearMagicPixels();

protected:
  float pixelCmp(ARGB32 a, ARGB32 b);

private:
  float tolerance;
  TList<ARGB32> samples;
};

#endif
