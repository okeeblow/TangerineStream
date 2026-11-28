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

#include "gradientwnd.h"

#include <bfc/canvas.h>

// NOTE
// works now ;)

GradientWnd::GradientWnd() {
  last_w = last_h = -1;
  recreate = 1;
  bitmap = NULL;
  setReverseColors(TRUE);
}

GradientWnd::~GradientWnd() {
  delete bitmap;
}

int GradientWnd::onPaint(Canvas *canvas) {
  ASSERT(canvas != NULL);
  RECT cr = clientRect();
  int w = cr.right - cr.left, h = cr.bottom - cr.top;
  if (w != last_w || h != last_h) {
    delete bitmap;
    bitmap = new BltCanvas(w, h, 32);
    recreate = 1;
    last_w = w;
    last_h = h;
  }

  if (recreate) {
    ARGB32 *bits = static_cast<ARGB32*>(bitmap->getBits());
    renderGradient(bits, w, h);
  }

  bitmap->getSkinBitmap()->blitAlpha(canvas, cr.left, cr.top, getPaintingAlpha());
  recreate = 0;

  return 1;
}

void GradientWnd::onParamChange() {
  invalidate();
  recreate = 1;
}
