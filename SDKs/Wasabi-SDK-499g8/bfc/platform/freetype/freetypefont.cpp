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
// ============================================================================================================================================================
// Font abstract class + statics to install TT fonts and Bitmap fonts
// ============================================================================================================================================================
#include "freetypefont.h"
#include "../../canvas.h"
#include "../../bitmap.h"
#include "bfc/attribs/cfgitem.h"

#define DO_KERNING // because Keith say so.

// local prototypes
const char *find_break( int (*width_func)(void *, const char *, int, int ), void *f, const char *str, int width );

#define M_PI 3.14159
#define M_2PI (M_PI*2)

#define FAUX_BOLD_RATIO 0.1f
#define FAUX_ITALIC_DEGREES 12

// This was necessary in Freetype 2.1.3 and bellow, but let us rejoice, they've fixed it
//#define NEED_SMALL_KERNING_HACK

int freetype_width( void *data, const char *str, int len, int fixed=0 );
int freetype_width( void *data, WCHAR *str, int len, int fixed=0 );


/**********************************************************************
 *
 *  FreeType Lib
 *
 **********************************************************************/

FreeTypeFont::FreeTypeFont() {
  fontbuffer = NULL;
  font = NULL;
  curboldstrength = 2;
}

int FreeTypeFont::addFontResource( FILE *file, const char *name ) {
  optionsitem = NULL;
  if ( FT_Init_FreeType( &flib ) ) {
    DebugString( "FreeType: Cannot load freetype!\n" );
    return 0;
  }

  last_encoding = -1;

  fontbufferlen = FGETSIZE( file );
  fontbuffer = (char *)MALLOC( fontbufferlen );
  FREAD( fontbuffer, fontbufferlen, 1, file );

  facename = name;

  if ( FT_New_Memory_Face( flib, (FT_Byte *)fontbuffer, fontbufferlen, 0, &font ) ) {
    DebugString( "FreeType: Cannot load font!\n" );
    return 0;
  }

  updateCharmap();

  return 1;
}

FreeTypeFont::~FreeTypeFont() {
  if ( fontbuffer ) {
    FREE( fontbuffer );
    FT_Done_Face(font);
    FT_Done_FreeType( flib );
  }
}

void FreeTypeFont::updateCharmap() {
  if (optionsitem == NULL) {
    const GUID options_guid = 
    { 0x280876cf, 0x48c0, 0x40bc, { 0x8e, 0x86, 0x73, 0xce, 0x6b, 0xb4, 0x62, 0xe5 } };
    optionsitem = WASABI_API_CONFIG->config_getCfgItemByGuid(options_guid);
  }
  int i = 0;
  if (optionsitem) i = _intVal(optionsitem, "Character mapping", 0);
  if (i != last_encoding) {
    FT_Done_Face(font);
    FT_Done_FreeType( flib );
    FT_Init_FreeType( &flib );
    FT_New_Memory_Face( flib, (FT_Byte *)fontbuffer, fontbufferlen, 0, &font );
    switch (i) {
      case -1:
        break;
      case 0:
        FT_Select_Charmap( font, FT_ENCODING_UNICODE);
        break;
      case 1:
        FT_Select_Charmap( font, FT_ENCODING_APPLE_ROMAN );
        break;
      case 2:
        FT_Select_Charmap( font, FT_ENCODING_ADOBE_LATIN_1 );
        break;
      case 3:
        FT_Select_Charmap( font, FT_ENCODING_ADOBE_STANDARD );
        break;
      case 4:
        FT_Select_Charmap( font, FT_ENCODING_ADOBE_CUSTOM );
        break;
      case 5:
        FT_Select_Charmap( font, FT_ENCODING_ADOBE_EXPERT );
        break;
      case 6:
        FT_Select_Charmap( font, FT_ENCODING_SJIS);
        break;
      case 7:
        FT_Select_Charmap( font, FT_ENCODING_BIG5);
        break;
      case 8:
        FT_Select_Charmap( font, FT_ENCODING_WANSUNG);
        break;
      case 9:
        FT_Select_Charmap( font, FT_ENCODING_JOHAB);
        break;
    }
  }
  last_encoding = i;
}

int FreeTypeFont::tweakSize(const char *face, int size) {
  if (STRCASEEQLSAFE(face, "nonstep")) return size-1;
  if (STRCASEEQLSAFE(face, "04B_08__")) return size+3;
  if (STRCASEEQLSAFE(face, "Blocky")) return size+6;
  if (STRCASEEQLSAFE(face, "04b_03b_")) return size+3;
  if (STRCASEEQLSAFE(face, "04b_09__")) return size+3;
  if (STRCASEEQLSAFE(face, "04b_21")) return size+3;
  if (STRCASEEQLSAFE(face, "Radiosta")) return size+2;
  if (STRCASEEQLSAFE(face, "ETHNOCENTRIC")) return size+3;
  if (STRCASEEQLSAFE(face, "ETHNOCEN")) return size+3;
  if (STRCASEEQLSAFE(face, "pixel")) return size+3;
  return size;
}

#define VERTICAL_TPADDING       2
#define VERTICAL_BPADDING       0
#define HORIZONTAL_LPADDING    -1
#define HORIZONTAL_RPADDING    1

void FreeTypeFont::prepareCanvas( CanvasBase *c, int size, int bold, int opaque, int underline, int italic, COLORREF color, COLORREF bkcolor, const char *txt, int width, int height ) {
#ifdef WIN32
  // Our "size" variable is fun to calculate!
  //int vRez = GetDeviceCaps(c->getHDC(), LOGPIXELSY); // this needs to be a Canvas method or something.
  int fsize = tweakSize(facename, size);
  int nHeight = MulDiv(fsize << 6, 72, 96);
  FT_Set_Char_Size( font, 0, nHeight, 0, 0 );  
#else
  FT_Set_Char_Size( font, 0, size << 6, 0, 0 );  
#endif

  updateCharmap();

  font->style_flags = 0;
  if ( bold ) {
    font->style_flags |= FT_STYLE_FLAG_BOLD;
    curboldstrength = (bold && c->getTextAntialias()) ? 2 : bold;
  }
  if ( italic )
    font->style_flags |= FT_STYLE_FLAG_ITALIC;
  if ( underline )
    font->underline_thickness = 1;
  else
    font->underline_thickness = 0;

  if ( height == -1 || width == -1 ) {
    getTextExtent( c, txt, &width, &height, size, bold, underline, italic, 0 );
  }

  blt = new SkinBitmap( width+1, height, color & 0xffffff );
  blt->setHasAlpha( 1 );
}

void FreeTypeFont::restoreCanvas( CanvasBase *c, int x, int y ) {
  unsigned int *bits = (unsigned int *)blt->getBits();
  int n = blt->getWidth() * blt->getHeight();

#ifndef NO_MMX
  if ( Blenders::MMX_AVAILABLE() ) 
    for ( int i = 0; i < n; i++ )
      *bits++ = Blenders::BLEND_MUL_MMX( *bits | 0xff000000, *bits >> 24 );
  else
#endif
    for ( int i = 0; i < n; i++ )
      *bits++ = Blenders::BLEND_MUL( *bits | 0xff000000, *bits >> 24 );

#ifndef NO_MMX
  Blenders::BLEND_MMX_END();
#endif

  blt->blit( c, x + HORIZONTAL_LPADDING, y + VERTICAL_TPADDING );
  delete blt; 
  blt = NULL;
}

int FreeTypeFont::getAscent() {
  FT_Glyph glyph;
  FT_BBox box;

  FT_Load_Glyph( font, FT_Get_Char_Index( font, 'M' ), FT_LOAD_DEFAULT );
  FT_Get_Glyph( font->glyph, &glyph );
  FT_Glyph_Get_CBox( glyph, ft_glyph_bbox_unscaled, &box );
  FT_Done_Glyph( glyph );

  return box.yMax >> 6;
}

void FreeTypeFont::drawText(int x, int y, const char *txt, int len, COLORREF color) {
  POINT pen = { x << 6, y};
  
  // we start left to right
  direction = 1;

  WCHAR *neutral = NULL;
  WCHAR *rtlstr = NULL;

  WCHAR *wtxt = (WCHAR *)MALLOC(sizeof(WCHAR)*(len+1));
  WCHAR *freeme = wtxt;
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, txt, len, wtxt, len);

  lastchar = 0;
  for (int i=0 ; *wtxt && i<len; ) {
    WORD cur_dir;
    GetStringTypeExW(LOCALE_SYSTEM_DEFAULT, CT_CTYPE2, wtxt, 1, &cur_dir);
    if (cur_dir == C2_RIGHTTOLEFT) {
      // we're now about to write some right-to-left text
      // we need to scan the string to determine the length of this right-to-left block
      if (!neutral) { neutral = (WCHAR *)MALLOC(sizeof(WCHAR)*(len+1)); }
      if (!rtlstr) { rtlstr = (WCHAR *)MALLOC(sizeof(WCHAR)*(len+1)); }
      rtlstr[0] = 0;
      neutral[0] = 0;

      WCHAR *p = wtxt;
      while (1) {
        WORD char_dir;
        if (*p) {
          GetStringTypeExW(LOCALE_SYSTEM_DEFAULT, CT_CTYPE2, p, 1, &char_dir);
          if (char_dir != C2_RIGHTTOLEFT && char_dir != C2_LEFTTORIGHT) {
            int l = WSTRLEN(neutral);
            neutral[l] = *p;
            neutral[l+1] = 0;
          }
        }
        if (!*p || char_dir == C2_LEFTTORIGHT) {
          // we now need to write rtlstr as right-to-left
          int w = freetype_width(font, rtlstr, WSTRLEN(rtlstr), 1); // this is in fixed point units

          // save current pen position
          int oldpenx = pen.x; 

          // jump to the end of the block
          pen.x += w;

          p = rtlstr;

          direction = -1;

          while (*p) {
            // move to the left by the width of the char
            pen.x -= freetype_width(font, p, 1, 1);
            // render the rtl character
            drawChar( pen.x, pen.y, *p, color );
            p++;
          }

          // now jump our to the end of the rtl block
          pen.x = oldpenx + w;

          // skip what we just printed from the source string
          wtxt += WSTRLEN(rtlstr);
          
          // and continue like nothing happened
          direction = 1;

          break;
        } else if (char_dir == C2_RIGHTTOLEFT) {
          WSTRCAT(rtlstr, neutral);
          *neutral = 0;
          int l = WSTRLEN(rtlstr);
          rtlstr[l] = *p;
          rtlstr[l+1] = 0;
        }
        p++;
      }
    } else {
      pen.x += drawChar( pen.x, pen.y, *wtxt, color );
      wtxt++;
    }
  }
  if (rtlstr) FREE(rtlstr);
  if (neutral) FREE(neutral);
  if (freeme) FREE(freeme);
}


int FreeTypeFont::drawChar( int x0, int y0, wchar_t c, COLORREF color ) {
  unsigned int *bits = (unsigned int *)blt->getBits();
  int width = blt->getWidth();
  int height = blt->getHeight();

  x0;

  FT_BitmapGlyph ftg;

  int glyph_index = FT_Get_Char_Index( font, c );

  FT_Vector delta = { 0, 0 };
#ifdef DO_KERNING
  if ( lastchar && FT_HAS_KERNING( font ) ) {
    FT_Get_Kerning( font, lastchar, glyph_index, FT_KERNING_DEFAULT, &delta );
    x0 += delta.x;
  }
#endif

  int rc = FT_Load_Glyph( font, glyph_index, FT_LOAD_DEFAULT|FT_LOAD_NO_BITMAP);
  if (rc)
    return 0;

  rc = FT_Get_Glyph( font->glyph, (FT_Glyph*)&ftg );
  if (rc)
    return 0;

  if (font->style_flags & FT_STYLE_FLAG_ITALIC) {

    FT_Matrix mat;
  
    // sets up the matrix, 0 degrees
    double sintheta = sin(0);    
    double costheta = cos(0);
    mat.xx = (FT_Fixed)(costheta * (1<<16));
    mat.xy = (FT_Fixed)(sintheta * (1<<16));
    mat.yx = -mat.xy;
    mat.yy = mat.xx;
  
    // shear the vectors for italic, 10 to 12 deg suggested
    FT_Fixed f = (FT_Fixed)(tan(M_2PI/(360/FAUX_ITALIC_DEGREES)) * (1<<16)); 
    mat.xy += FT_MulFix(f, mat.xx);
    mat.yy += FT_MulFix(f, mat.yx);

    // do the transform
    FT_Vector v = {0,0};
    FT_Vector_Transform(&v, &mat);
    FT_Glyph_Transform((FT_Glyph)ftg, &mat, &v);
  
  }

  // get the glyph
  rc = FT_Glyph_To_Bitmap( (FT_Glyph*)&ftg, ft_render_mode_normal, NULL, 1 );
  if (rc) {
    FT_Done_Glyph( (FT_Glyph)ftg );
    return 0;
  }

  FT_Bitmap *bmp = &ftg->bitmap;

  int r = 1;
  if (font->style_flags & FT_STYLE_FLAG_BOLD) 
    r = MAX((int)((float)(font->glyph->advance.x >> 6) * FAUX_BOLD_RATIO), 2);

  int ys = MAX( 0, y0 - ftg->top );
  int ye = MIN( height, y0 + bmp->rows - ftg->top );
  int xs = MAX( 0, (x0 >> 6) + ftg->left );
  int xe = MIN( width, (x0 >> 6) + ftg->left + bmp->width );

#ifdef NEED_SMALL_KERNING_HACK
  int o = 0; xe-xs >= 3 && xe-xs <= 10 ? MAX(0, ftg->left) : 0;
#else
  int o = 0; 
#endif

  unsigned char *_bmpbits = bmp->buffer + (ys + ftg->top - y0 ) * bmp->pitch + (xs - ftg->left - (x0 >> 6));
  unsigned int *_linebits = (unsigned int *)(bits + ys * width + xs - o);

  if (ftg->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
    for (int z = 0; z < r; z++ ) {
      unsigned char *bmpbits = _bmpbits;
      unsigned int *linebits = _linebits + z;
      for ( int y = ys; y < ye; y++ ) {
        if (z > 0) {
          for ( int x = xs; x < xe; x++ ) {
            if (x != width-z) {
              *linebits |= overlay((int)(*linebits >> 24), (int)*bmpbits, curboldstrength, r, z) << 24;
              linebits++; bmpbits++;
            } else {
              linebits++; bmpbits++;
            }
          }
        } else {
          for ( int x = xs; x < xe; x++ )
            *linebits++ |= *bmpbits++ << 24;
        }
        bmpbits += bmp->pitch - (xe - xs);
        linebits += width - (xe - xs);
      }
    }
  } else if (ftg->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
    for (int z = 0; z < r; z++ ) {
      unsigned char *bmpbits = _bmpbits;
      unsigned int *linebits = _linebits + z;
      for ( int y = ys; y < ye; y++ ) {
        for ( int x = xs; x < xe; x++ ) {
/*          if (y == ys || y == ye-1 || x == xs || x == xe-1)
            *linebits++ |= 0xFF << 24;
          else
            linebits++;*/
          int byte = (x-xs)>>3;
          *linebits++ |= (*(bmpbits + byte) & (1 << (7-((x-xs)-(byte<<3)))) ? 0xFF : 0) << 24;
        }
        bmpbits += bmp->pitch;
        linebits += width - (xe - xs);
      }
    }
  } else {
    // simply draw rectangles
    for (int z = 0; z < r; z++ ) {
      unsigned char *bmpbits = _bmpbits;
      unsigned int *linebits = _linebits + z;
      for ( int y = ys; y < ye; y++ ) {
        for ( int x = xs; x < xe; x++ ) {
          if (y == ys || y == ye-1 || x == xs || x == xe-1)
            *linebits++ |= 0xFF << 24;
          else
            linebits++;
        }
        bmpbits += bmp->pitch;
        linebits += width - (xe - xs);
      }
    }
  }


  FT_Done_Glyph( (FT_Glyph)ftg );

  lastchar = glyph_index;
  
  return font->glyph->advance.x + delta.x + ((r - 1) << 6) - (o << 6);
}

void FreeTypeFont::textOut(CanvasBase *c, int x, int y, const char *txt, int size, int bold, int opaque, int underline, int italic, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias) {
  color = RGBTOBGR(color);
  bkcolor = RGBTOBGR(bkcolor);
  y++; x++;
  prepareCanvas( c, size, bold, opaque, underline, italic, color, bkcolor, txt );
  int maxheight = getAscent();

  drawText(0, maxheight, txt, STRLEN(txt), color);

/*  POINT pen = { 0, maxheight };

  lastchar = 0;
  for ( ; *txt; txt++ ) {
    pen.x += drawChar( pen.x, pen.y, *txt, color );
  }*/
  
  restoreCanvas( c, x + xoffset , y + yoffset );
}

void FreeTypeFont::textOut2(CanvasBase *c, int x, int y, int w, int h, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias) {
//  if (STRCASEEQL(txt, "Equalizer!")) __asm int 3;
  color = RGBTOBGR(color);
  bkcolor = RGBTOBGR(bkcolor);
  y++; x++;
  prepareCanvas( c, size, bold, opaque, underline, italic, color, bkcolor, txt, w, h );
  int maxheight = getAscent();
  int width = getTextWidth(c,txt,size,bold,underline,italic,antialias);

  int xstart = 0;
  if ( align == DT_RIGHT ) {
    xstart = w - width;
  } else if ( align == DT_CENTER ) {
    xstart = (w - width) / 2;
  }


  drawText(xstart, maxheight, txt, STRLEN(txt), color);

/*  POINT pen = { xstart << 6, maxheight };

  lastchar = 0;
  for ( ; *txt; txt++ ) {
    pen.x += drawChar( pen.x, pen.y, *txt, color );
  }*/
  
  restoreCanvas( c, x + xoffset , y + yoffset );
}

void FreeTypeFont::textOutEllipsed(CanvasBase *c, int x, int y, int w, int h, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias) {
  color = RGBTOBGR(color);
  bkcolor = RGBTOBGR(bkcolor);
  y++; x++;
  if ( txt == NULL ) 
    return;

  RECT r;
  r.left = x+xoffset;
  r.top = y+yoffset;
  r.right = r.left + w;
  r.bottom = r.top + h;
  prepareCanvas( c, size, bold, opaque, underline, italic, color, bkcolor, txt, w, h );

  char *tmp = (char *)MALLOC( STRLEN( txt ) + 3 );
  STRCPY( tmp, txt );
  int width, height;
  width = getTextWidth( c, txt, size, bold, underline, italic, antialias );
  height = getAscent();
  int dddw = getTextWidth(c,"...", size, bold, underline, italic, antialias);

  if ( width > r.right - r.left ) {
    char *p = tmp + STRLEN( tmp );
    width = r.right - r.left - dddw;
    while( p > tmp &&
	   getTextWidth(c,tmp,size,bold,underline,italic,antialias) > width ) {
      *p-- = '\0';
    }
    STRCPY( p, "..." );
  }

  drawText(0, height, tmp, STRLEN(tmp), color);

/*  POINT pen = { 0, height };

  lastchar = 0;
  for ( char *p = tmp; *p; p++ ) {
    pen.x += drawChar( pen.x, pen.y, *p, color );
  }*/

  FREE( tmp );

  restoreCanvas( c, r.left, r.top );
}

int freetype_width( void *data, const char *str, int len, int fixed ) {
  WCHAR *wtxt = (WCHAR *)MALLOC(sizeof(WCHAR)*(len+1));
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, len, wtxt, len);

  int l = freetype_width(data, wtxt, len, fixed);

  FREE(wtxt);
  return l;
}

int freetype_width( void *data, WCHAR *str, int len, int fixed ) {
  FT_Face font = (FT_Face)data;
  
  int w = 0;

  int prev, index;
  WCHAR *p;
  int count = 0;
  for ( p = str; *p && count < len; ) {
    FT_Vector delta = { 0, 0 };

    FT_BitmapGlyph ftg;

    index = FT_Get_Char_Index( font, *p );
    
#ifdef DO_KERNING
    if ( w > 0 && FT_HAS_KERNING( font ) ) {
      FT_Get_Kerning( font, prev, index, ft_kerning_default, &delta );
    }
#endif

    int rc = FT_Load_Glyph( font, index, FT_LOAD_DEFAULT|FT_LOAD_NO_BITMAP );
    if (rc) {
      p++;
      count++;
      continue;
    }

    w += (delta.x + font->glyph->advance.x); 
    prev = index;


    rc = FT_Get_Glyph( font->glyph, (FT_Glyph*)&ftg );
    if (rc) {
      p++;
      count++;
      continue;
    }

    if (font->style_flags & FT_STYLE_FLAG_ITALIC) {

      FT_Matrix mat;
  
      // sets up the matrix, 0 degrees
      double sintheta = sin(0);    
      double costheta = cos(0);
      mat.xx = (FT_Fixed)(costheta * (1<<16));
      mat.xy = (FT_Fixed)(sintheta * (1<<16));
      mat.yx = -mat.xy;
      mat.yy = mat.xx;
  
      // shear the vectors for italic, 10 to 12 deg suggested
      FT_Fixed f = (FT_Fixed)(tan(M_2PI/(360/FAUX_ITALIC_DEGREES)) * (1<<16)); 
      mat.xy += FT_MulFix(f, mat.xx);
      mat.yy += FT_MulFix(f, mat.yx);

      // do the transform
      FT_Vector v = {0,0};
      FT_Vector_Transform(&v, &mat);
      FT_Glyph_Transform((FT_Glyph)ftg, &mat, &v);
  
    }

    // get the glyph
    rc = FT_Glyph_To_Bitmap( (FT_Glyph*)&ftg, ft_render_mode_normal, NULL, 1 );
    if (rc) {
      FT_Done_Glyph( (FT_Glyph)ftg );
      p++;
      count++;
      continue;
    }
    FT_Bitmap *bmp = &ftg->bitmap;

    int ys = MAX( 0, ftg->top );
    int ye = bmp->rows - ftg->top;
    int xs = MAX( 0, ftg->left );
    int xe = ftg->left + bmp->width;

#ifdef NEED_SMALL_KERNING_HACK
    int o = xe-xs >= 3 && xe-xs <= 10 ? MAX(0, ftg->left) : 0;
#else
    int o = 0;
#endif

    FT_Done_Glyph( (FT_Glyph)ftg );

    int r = 1;
    if (font->style_flags & FT_STYLE_FLAG_BOLD) {
      r = MAX((int)((float)(font->glyph->advance.x >> 6) * FAUX_BOLD_RATIO), 2);
    }
    w += ((r - 1) << 6) - (o << 6);
    p++;
    count++;
  }

  if (fixed) return w;
  return (w >> 6);
}


void FreeTypeFont::textOutWrapped(CanvasBase *c, int x, int y, int w, int h, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias) {
  color = RGBTOBGR(color);
  bkcolor = RGBTOBGR(bkcolor);
  y++; x++;
  prepareCanvas( c, size, bold, opaque, underline, italic, color, bkcolor, txt, w, h );

  int ascent = getAscent();
  int descent = getTextHeight2( c, size, bold, underline, italic, antialias );
  descent -= ascent;
  
  int yoff = ascent;
  const char *cur = txt, *next;
  int length = STRLEN( txt );
  
  //NO.
  //for( int yoff = ascent; 
  for( yoff = ascent; 
       yoff < h; 
       yoff += ascent + descent ) {

    next = find_break( freetype_width, font, cur, w );
    
/*    POINT pen = { 0, yoff };
    
    lastchar = 0;
    for ( ; cur < next; cur++ ) {
      pen.x += drawChar( pen.x, pen.y, *cur, color );
    }*/
    drawText(0, yoff, cur, next-cur, color);
    
    cur = next;
    while (*cur && *cur == ' ')
      cur++;
    
    if (*cur == '\r' || *cur == '\n') cur++;
    
    if ( cur >= txt + length )
      break;
  }

  restoreCanvas( c, x + xoffset , y + yoffset );
}

void FreeTypeFont::textOutWrappedPathed(CanvasBase *c, int x, int y, int w, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias) {
  color = RGBTOBGR(color);
  bkcolor = RGBTOBGR(bkcolor);
  DebugString( "writeme -- FreeTypeFont::textOutWrappedPathed...\n" );
}

void FreeTypeFont::textOutCentered(CanvasBase *c, RECT *r, const char *txt, int size, int bold, int opaque, int underline, int italic, int align, COLORREF color, COLORREF bkcolor, int xoffset, int yoffset, int antialias) {
  color = RGBTOBGR(color);
  bkcolor = RGBTOBGR(bkcolor);
  r->top++;
  r->left++;

  RECT rr = *r;
  rr.left += xoffset;
  rr.right += xoffset;
  rr.top += yoffset;
  rr.bottom += yoffset;

  prepareCanvas( c, size, bold, opaque, underline, italic, color, bkcolor, txt, rr.right - rr.left, rr.bottom - rr.top );

  int width, height;
  height = getAscent();
  width = getTextWidth( c, txt, size, bold, underline, italic, antialias );



  drawText( ((rr.right - rr.left - width) / 2), height, txt, STRLEN(txt), color);
/*  POINT pen = { ((rr.right - rr.left - width) / 2) << 6, height };

  lastchar = 0;
  for ( ; *txt; txt++ ) {
    pen.x += drawChar( pen.x, pen.y, *txt, color );
  }*/
  
  restoreCanvas( c, rr.left, rr.top );
}

int FreeTypeFont::getTextWidth(CanvasBase *c, const char *text, int size, int bold, int underline, int italic, int antialiased) {
  int w;
  updateCharmap();
  getTextExtent(c, text, &w, NULL, size, bold, underline, italic, antialiased);
  return w;
}

int FreeTypeFont::getTextHeight(CanvasBase *c, const char *text, int size, int bold, int underline, int italic, int antialiased) {
  int h;
  updateCharmap();
  getTextExtent(c, text, NULL, &h, size, bold, underline, italic, antialiased);
  {
    // calcul for multiline text
    const char *p=text;
    int n=0;
    while(*p!=0) if(*p++=='\n') n++;
    if(n) h*=(n+1);
  }
  return h;
}

int FreeTypeFont::getTextHeight2(CanvasBase *c, int size, int bold, int underline, int italic, int antialiased) {
  return getTextHeight(c, "Mg", size, bold, underline, italic, antialiased);
}

void FreeTypeFont::getTextExtent( CanvasBase *c, const char *txt, int *w, int *h, int size, int bold, int underline, int italic, int antialias ) {  
  updateCharmap();
#ifdef WIN32
  // Our "size" variable is fun to calculate!
  int fsize = tweakSize(facename, size);
  //int vRez = GetDeviceCaps(c->getHDC(), LOGPIXELSY); // this needs to be a Canvas method or something.
  int nHeight = MulDiv(fsize << 6, 72, 96);
  FT_Set_Char_Size( font, 0, nHeight, 0, 0 );  
#else
  FT_Set_Char_Size( font, 0, size << 6, 0, 0 );  
#endif

  font->style_flags = 0;
  if ( bold )
    font->style_flags |= FT_STYLE_FLAG_BOLD;
  if ( italic )
    font->style_flags |= FT_STYLE_FLAG_ITALIC;
  if ( underline )
    font->underline_thickness = 1;
  else
    font->underline_thickness = 0;

  SIZE rsize={0,0};
  ASSERT(txt != NULL);
  if (*txt == 0) {
    if (w != NULL) *w = 0;
    if (h != NULL) *h = 0;
    return;
  }

  FT_BBox box;
  FT_Glyph glyph;

  int minh = 0, maxh = 0;

  if ( w ) *w = freetype_width( font, txt, STRLEN( txt ) ) + HORIZONTAL_RPADDING + HORIZONTAL_LPADDING;

  if ( h ) {
    FT_Load_Glyph( font, FT_Get_Char_Index( font, 'M' ), FT_LOAD_DEFAULT );
    FT_Get_Glyph( font->glyph, &glyph );
    FT_Glyph_Get_CBox( glyph, ft_glyph_bbox_unscaled, &box );
    maxh = box.yMax;
    FT_Done_Glyph( glyph );

    FT_Load_Glyph( font, FT_Get_Char_Index( font, 'g' ), FT_LOAD_DEFAULT );
    FT_Get_Glyph( font->glyph, &glyph );
    FT_Glyph_Get_CBox( glyph, ft_glyph_bbox_unscaled, &box );
    minh = box.yMin;
    FT_Done_Glyph( glyph );

    *h = ((maxh - minh) >> 6) + VERTICAL_TPADDING + VERTICAL_BPADDING;
  }
}


int FreeTypeFont::isBitmap() {
  return 0;
}

const char *find_break( int (*width_func)(void *, const char *, int, int ), 
			void *f, const char *str, int width ) {
  const char *softret, *lastsoft, *hardret;

  if ( width_func( f, str, STRLEN( str ), 0 ) <= width )
    return str + STRLEN( str );

  for( hardret = str; *hardret; hardret ++ )
    if ( *hardret == '\r' || *hardret == '\n' )
      break;

  if ( hardret && width_func( f, str, hardret - str, 0 ) <= width ) {
    return hardret;
  }
  for( softret = str; *softret && !isspace( *softret ); softret++ )
    ;
  
  if ( width_func( f, str, softret - str, 0 ) <= width ) {
    do {
      lastsoft = softret;

      for( softret = lastsoft+1; *softret && !isspace( *softret ); softret++ )
	;
      
    } while ( *lastsoft && width_func( f, str, softret - str, 0 ) <= width );

    softret = lastsoft;
  } else {
    for( softret = str; *softret; softret++ )
      if ( width_func( f, str, softret - str, 0 ) > width )
	break;

    softret--;
  }

  return softret;
}
