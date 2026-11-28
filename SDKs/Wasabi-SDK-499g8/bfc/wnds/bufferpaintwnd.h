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
#ifndef __BPAINTWND_H
#define __BPAINTWND_H

#include "../../common/guiobjwnd.h"
#include "../../bfc/autobitmap.h"

#define BUFFERPAINTWND_PARENT GuiObjectWnd 

class COMEXP BufferPaintWnd : public BUFFERPAINTWND_PARENT {
  
  public:
    BufferPaintWnd();
    virtual ~BufferPaintWnd();

    virtual int onInit(); 
    virtual int onPaint(Canvas *c); 
  
    virtual int onBufferPaint(BltCanvas *c, int w, int h) { return 1; } 
    virtual int wantEvenAlignment() { return 0; } // if you need even coordinates for your framebuffer, return 1 here
    virtual void getBufferPaintSize(int *w, int *h); // by default returns client width/height
    virtual void getBufferPaintSource(RECT *r); // by default returns the size of the quickpaint canvas
    virtual void getBufferPaintDest(RECT *r); // by default returns the size of client area
    virtual int wantNegativeHeight() { return 0; }
    virtual void invalidateBuffer();
    virtual int onResize();
 
  protected:
    BltCanvas *render_canvas;

  private:
    void bufferPaint();
    int updateCanvas();

    int canvas_w, canvas_h;

    int invalidated;
};


#endif
