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
#ifndef _VIDEO_OUT_H
#define _VIDEO_OUT_H

class VideoWnd;

class VideoOutput {
public:
  virtual ~VideoOutput() { }
  
  virtual int create(VideoWnd *parent, int w, int h, const char *type, int flipit, float aspectratio)=0; //return 1 if ok
  virtual int needChange()=0; //return 1 if need to renegociate video output

  virtual int onPaint(Canvas *canvas, RECT &r) { return 0; } //return 1 if override
  virtual void displayFrame(const char *buf, int size, int time)=0;

  virtual void goFullScreen()=0;
  virtual void removeFullScreen()=0;
  virtual int showOSD() { return 0; }
  virtual void hideOSD() { }

  virtual void timerCallback() { }

  virtual void setPalette(RGBQUAD *pal) { }
};

#endif