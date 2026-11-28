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
#ifndef __DDRAWWND_H
#define __DDRAWWND_H

#include <ddraw.h>
#include "../bfc/basewnd.h"

class DDSurfaceCanvas;
class DDrawWnd;
class Region;

#define DDRAWWND_PARENT BaseWnd

class COMEXP NOVTABLE DDrawWnd : public DDRAWWND_PARENT {

public:

  DDrawWnd();
  virtual ~DDrawWnd();

  virtual int virtualBeforePaint(Region *r);
  virtual int virtualAfterPaint(Region *r);

  virtual void virtualCanvasCommit(Canvas *canvas, RECT *r, double ratio);

  virtual Canvas *createFrameBuffer(int w, int h);
  virtual void deleteFrameBuffer(Canvas *canvas);

  virtual int onInit();

  virtual void invalidate();
  virtual void invalidateRect(RECT *r);
  virtual void invalidateRgn(Region *rgn);
  virtual void validate();
  virtual void validateRect(RECT *r);
  virtual void validateRgn(Region *rgn);

private:

  void initDDraw();
  void startThread();
  void stopThread();

  LPDIRECTDRAW m_lpDD;
  LPDIRECTDRAWSURFACE m_lpRenderSurf, m_lpPrimSurf;

  DDSurfaceCanvas *fb_canvas;
  int w, h;
  LPDIRECTDRAWCLIPPER lpClipper;
  static int allow_dd;
  static int sleep_val;
  static CRITICAL_SECTION cs;
  static HANDLE thread;
  static int quitthread;
  static PtrList<DDrawWnd> ddlist;

  static unsigned int WINAPI renderThread(void *);
};

#endif

