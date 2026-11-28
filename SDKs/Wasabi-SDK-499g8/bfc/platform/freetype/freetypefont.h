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
#ifndef __FREETYPEFONT_H
#define __FREETYPEFONT_H

#include "../../../studio/services/svc_font.h"
#include "../../../bfc/blending.h"
#include "../../../bfc/string.h"
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>

class CanvasBase;
class SkinBitmap;

class FreeTypeFont : public svc_fontI {
 public:
  FreeTypeFont();
  virtual ~FreeTypeFont();

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

  virtual int isBitmap();
  virtual const char *getFaceName() { return facename; }
  virtual void setFontFace(const char *face) { facename = face; }

  virtual void setFontId(const char *id) { font_id = id; }
  virtual const char *getFontId() { return font_id; }
  virtual int getScriptId() { return scriptid; }
  virtual void setScriptId(int id) { scriptid = id; }

  virtual const char *getFontSvcName() { return "Freetype"; }
  static const char *getServiceName() { return "Freetype Font Service"; }

 protected:
  
  int addFontResource( FILE *font, const char *name );
  
  void prepareCanvas(CanvasBase *c, int size, int bold, int opaque, int underline, int italic, COLORREF color, COLORREF bkcolor, const char *txt, int width = -1, int height = -1 );
  void restoreCanvas(CanvasBase *c, int x, int y );
  int drawChar( int x, int y, wchar_t c, COLORREF color );
  int getAscent();

 protected:
  String font_id;
  String facename;
  int scriptid;

 private:
  void updateCharmap();
  void drawText(int x, int y, const char *txt, int len, COLORREF color);
  int tweakSize(const char *face, int size);
  char *FreeTypeFont::filenameToFontFace(const char *pszFile);
  inline unsigned char overlay(unsigned char c, unsigned char b, int amount, int r, int z) {
    if (curboldstrength == 2 && z == r-1) {
      if (c < b) 
        return Blenders::BLEND_ADJ1(c, b, 0xB0);
      return MIN(255, c+b);;
    }
    return MIN(255, c+b);
  }
  FT_Library flib;
  char *fontbuffer;
  int fontbufferlen;
  FT_Face font;
  FT_Face font_bold;
  FT_Face font_italic;
  FT_Face font_bolditalic;
  SkinBitmap *blt;
  int lastchar;
  int curboldstrength;
  int cursize;
  int direction;
  int last_encoding;
  CfgItem *optionsitem;
};

#endif//__FREETYPEFONT_H
