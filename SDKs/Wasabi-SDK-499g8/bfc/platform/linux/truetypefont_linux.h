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
#ifndef __TRUETYPEFONT_LINUX_H
#define __TRUETYPEFONT_LINUX_H

#include "../../../studio/services/svc_font.h"

#include "../../string.h"
#include "../../stack.h"
#include "../truetypefontdef.h"

class CanvasBase;
class BltCanvas;

class TrueTypeFont_Linux : public svc_fontI {
 public:
  TrueTypeFont_Linux();
  virtual ~TrueTypeFont_Linux();

  virtual void textOut(CanvasBase *c, int x, int y, const char *txt, int size, int bold, int opaque, int underline, int italic, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias);
  virtual void textOut2(CanvasBase *c, int x, int y, int w, int h, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias);
  virtual void textOutEllipsed(CanvasBase *c, int x, int y, int w, int h, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias);
  virtual void textOutWrapped(CanvasBase *c, int x, int y, int w, int h, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias);
  virtual void textOutWrappedPathed(CanvasBase *c, int x, int y, int w, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias);
  virtual void textOutCentered(CanvasBase *c, RECT *r, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias);
  virtual int getTextWidth(CanvasBase *c, const char *text, int size, int bold, int underline, int italic, int antialias);
  virtual int getTextHeight(CanvasBase *c, const char *text, int size, int bold, int underline, int italic, int antialias);
  virtual int getTextHeight2(CanvasBase *c, int size, int bold, int underline, int italic, int antialias);
  virtual void getTextExtent(CanvasBase *c, const char *text, int *w, int *h, int size, int bold, int underline, int italic, int antialias);

  virtual void setFontId(const char *id) { font_id = id; }
  virtual const char *getFontId() { return font_id; }
  virtual int getScriptId() { return scriptid; }
  virtual void setScriptId(int id) { scriptid = id; }

  virtual int isBitmap();
  virtual void setFontFace(const char *face);
  virtual int addFontResource(FILE *f);
  virtual const char *getFaceName();

  virtual const char *getFontSvcName() { return "Linux"; }
  static const char *getServiceName() { return "Linux Font Service"; }

 protected:
  static char *filenameToFontFace(const char *filename);
  void prepareCanvas(CanvasBase *c, int size, int bold, int opaque, int underline, int italic, COLORREF color, COLORREF bkcolor);
  void restoreCanvas(CanvasBase *c);

 protected:
  String font_id;
  int scriptid;

 private:
  CanvasBase *prepareAntialias(CanvasBase *c, int x, int y, const char *txt, int size, int bold, int opaque, int underline, int italic, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int w, int h);
  void completeAntialias(CanvasBase *c);

  String face_name;
  String tmpfilename;
  int DColdstate;
  XFontStruct *font;
  Stack<fontslot*> fontstack;
  BltCanvas *antialias_canvas;
  int al_w, al_h, al_x, al_y, al_xo, al_yo, al_dw, al_dh;
  COLORREF al_mask;
};

#endif
