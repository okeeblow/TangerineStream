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
#ifndef _GRADIENT_H
#define _GRADIENT_H

#include <bfc/std.h>

class COMEXP Gradient {
public:
  Gradient();
  virtual ~Gradient();

  void setX1(float x1);
  void setY1(float y1);
  void setX2(float x2);
  void setY2(float y2);
  
  void clearPoints();
  void addPoint(float pos, ARGB32 color);

  // "pos=color;pos=color" "0.25=34,45,111"
  void setPoints(const char *str);

  void setReverseColors(int c);

  void setAntialias(int c);

  void setMode(const char *mode);

  void setGammaGroup(const char *group);

  // note: this will automatically premultiply against alpha
  void renderGradient(ARGB32 *bits, int width, int height);

protected:
  virtual void onParamChange() { }

  ARGB32 getPixelCirc(double x, double y);

private:
  float gradient_x1, gradient_y1, gradient_x2, gradient_y2;
  class GradientList;
  GradientList *list;
  void renderGrad(ARGB32 *bits, int len, int *positions);
  int reverse_colors;
  int antialias;
  String mode, gammagroup;
};

#endif
