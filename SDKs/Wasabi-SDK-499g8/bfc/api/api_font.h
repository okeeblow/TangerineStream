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
#ifndef __API_FONT_H
#define __API_FONT_H

#include "../dispatch.h"

class CanvasBase;

class api_font : public Dispatchable {
  public:
    void font_textOut(CanvasBase *c, int style, int x, int y, int w, int h, const char *txt);
    int font_getInfo(CanvasBase *c, const char *font, int infoid, const char *txt, int *w, int *h);

  enum {
    API_FONT_FONT_TEXTOUT = 0,
    API_FONT_FONT_GETINFO = 10,
  };
};

inline void api_font::font_textOut(CanvasBase *c, int style, int x, int y, int w, int h, const char *txt) {
  _voidcall(API_FONT_FONT_TEXTOUT, c, style, x, y, w, h, txt);
}

inline int api_font::font_getInfo(CanvasBase *c, const char *font, int infoid, const char *txt, int *w, int *h) {
  return _call(API_FONT_FONT_GETINFO, (int)0, c, font, infoid, txt, w, h);
}

class api_fontI : public api_font {
  public:
    virtual void font_textOut(CanvasBase *c, int style, int x, int y, int w, int h, const char *txt)=0;
    virtual int font_getInfo(CanvasBase *c, const char *font, int infoid, const char *txt, int *w, int *h)=0;

  protected:
    api_fontI() {}
    virtual ~api_fontI() {}

    RECVS_DISPATCH;
};

// {1FCA9C7E-5923-4b9c-8906-0F8C331DF21C}
static const GUID fontApiServiceGuid = 
{ 0x1fca9c7e, 0x5923, 0x4b9c, { 0x89, 0x6, 0xf, 0x8c, 0x33, 0x1d, 0xf2, 0x1c } };

extern api_font *fontApi;

#endif
