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
#ifndef _DRAWPOLY_H
#define _DRAWPOLY_H

#include "../std.h"

class COMEXP Draw {
public:
  static void beginPolygon(ARGB32 *bits, int w, int h, ARGB32 color);
  static void addPoint(int x, int y);
  static void endPolygon();
  static void drawPointList(ARGB32 *bits, int w, int h, const char *pointlist);
};

// x,y;x,y;x,y;x,y=R,G,B|x,y;x,y;x,y;=R,G,B

#endif
