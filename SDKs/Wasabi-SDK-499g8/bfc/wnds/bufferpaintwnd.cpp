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
#include "bufferpaintwnd.h"
#include "../../bfc/canvas.h"
#include "../../studio/api.h"

// -----------------------------------------------------------------------
BufferPaintWnd::BufferPaintWnd() {
  canvas_w = -1;
  canvas_h = -1;
  render_canvas = NULL;
  invalidated = 1;
}

// -----------------------------------------------------------------------
BufferPaintWnd::~BufferPaintWnd() {
  delete render_canvas;
}

// -----------------------------------------------------------------------
int BufferPaintWnd::onInit() {
  BUFFERPAINTWND_PARENT::onInit();
  return 1;
}

// -----------------------------------------------------------------------
void BufferPaintWnd::bufferPaint() {
  updateCanvas();
  if (render_canvas != NULL)
    onBufferPaint(render_canvas, canvas_w, canvas_h);
}

void BufferPaintWnd::invalidateBuffer() {
  invalidated = 1;
  invalidate();
}

// -----------------------------------------------------------------------
void BufferPaintWnd::getBufferPaintSize(int *w, int *h) {
  RECT r;
  getClientRect(&r);
  if (w) *w = r.right-r.left;
  if (h) *h = r.bottom-r.top;
}

// -----------------------------------------------------------------------
void BufferPaintWnd::getBufferPaintSource(RECT *r) {
  ASSERT(r != NULL);
  r->left = 0;
  r->right = canvas_w;
  r->top = 0;
  r->bottom = canvas_h;
}

// -----------------------------------------------------------------------
void BufferPaintWnd::getBufferPaintDest(RECT *r) {
  ASSERT(r != NULL);
  getClientRect(r);
}

// -----------------------------------------------------------------------
int BufferPaintWnd::onPaint(Canvas *canvas) {

  BUFFERPAINTWND_PARENT::onPaint(canvas);

  if (invalidated) bufferPaint();
  invalidated = 0;

  RECT r;
  getBufferPaintDest(&r);
  RECT sr;
  getBufferPaintSource(&sr);

  render_canvas->getSkinBitmap()->stretchToRectAlpha(canvas, &sr, &r, getPaintingAlpha());

  return 1;
}

// -----------------------------------------------------------------------
int BufferPaintWnd::onResize() {
  if (!BUFFERPAINTWND_PARENT::onResize()) return 0;
  if (updateCanvas()) {
    invalidated = 1;
    invalidate();
  }
  return 1;
}

// -----------------------------------------------------------------------
int BufferPaintWnd::updateCanvas() {
  int w, h;
  getBufferPaintSize(&w, &h);

  if (wantEvenAlignment()) {
    if (w & 1) w++;
    if (h & 1) h++;
  }

  if (w == 0 || h == 0) return 0; 

  int newone = 0;

  if (canvas_w != w || canvas_h != h) {
    delete render_canvas;
    render_canvas = new BltCanvas(w, wantNegativeHeight() ? -h : h);
    canvas_w = w;
    canvas_h = h;
    newone = 1;
  }

  return newone;
}
