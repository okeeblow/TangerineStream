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
#include "xuirect.h"

#include <bfc/canvas.h>
#include <bfc/paramparser.h>
#include <bfc/draw/skinfilter.h>

#define BLTSIZE 1

enum {
  SCRIPTRECT_SETCOLOR = 0,
  SCRIPTRECT_SETFILLED,
  SCRIPTRECT_EDGES,
  SCRIPTRECT_THICKNESS,
  SCRIPTRECT_GAMMAGROUP,
};
namespace RectEdges {
  enum { LEFT=1, RIGHT=2, TOP=4, BOTTOM=8 };
};
using namespace RectEdges;

ScriptRect::ScriptRect() {
  filled = 0;
  myxuihandle = newXuiHandle();
  color.setColorGroup("");
  
  addParam(myxuihandle, "color", SCRIPTRECT_SETCOLOR, XUI_ATTRIBUTE_REQUIRED);
  addParam(myxuihandle, "filled", SCRIPTRECT_SETFILLED, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "edges", SCRIPTRECT_EDGES, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "thickness", SCRIPTRECT_THICKNESS, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "gammagroup", SCRIPTRECT_GAMMAGROUP, XUI_ATTRIBUTE_IMPLIED);

  pixel = new BltCanvas(BLTSIZE,BLTSIZE,32);
  resetPixel();
  edges = LEFT | RIGHT | TOP | BOTTOM;
  thickness = 1;
}

ScriptRect::~ScriptRect() {
  delete pixel;
}

int ScriptRect::onInit() {
  SCRIPTRECT_PARENT::onInit();
  return 1;
}

int ScriptRect::onPaint(Canvas *c) {
  PaintCanvas pc;
  if (c == NULL) {
    if (!pc.beginPaint(this)) return 0;
    c = &pc;
  }

  // check for colors changing on us
  if (!color.iteratorValid()) resetPixel();

  RECT src={0,0,BLTSIZE,BLTSIZE};
  if (filled) {
    RECT dst;
    getClientRect(&dst);
    pixel->getSkinBitmap()->stretchToRectAlpha(c, &src, &dst, getPaintingAlpha());
  } else {
    RECT dst, odst;
    getClientRect(&odst);
    if (edges & TOP) {
      dst = odst;
      dst.bottom = dst.top+thickness;
      pixel->getSkinBitmap()->stretchToRectAlpha(c, &src, &dst, getPaintingAlpha());
    }
    if (edges & BOTTOM) {
      dst = odst;
      dst.top = dst.bottom-thickness;
      pixel->getSkinBitmap()->stretchToRectAlpha(c, &src, &dst, getPaintingAlpha());
    }
    if (edges & RIGHT) {
      dst = odst;
      dst.top++; dst.bottom--;
      dst.left = dst.right-thickness;
      pixel->getSkinBitmap()->stretchToRectAlpha(c, &src, &dst, getPaintingAlpha());
    }
    if (edges & LEFT) {
      dst = odst;
      dst.right = dst.left+thickness;
      pixel->getSkinBitmap()->stretchToRectAlpha(c, &src, &dst, getPaintingAlpha());
    }
  }

  return 1;
}

int ScriptRect::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTRECT_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch(xmlattributeid) {
    case SCRIPTRECT_SETCOLOR: {
      ARGB32 prev_color = color;
      color.setElementName(value);
//CUT      color = WASABI_API_SKIN->skin_getColorElement((char*)value);
      if (color.v() != prev_color) {
//CUT?        ApplySkinFilters::apply(NULL, getXmlParamByName("gammagroup"), &color, BLTSIZE, BLTSIZE);
        resetPixel();
        invalidate();
      }
    }
    break;
    case SCRIPTRECT_GAMMAGROUP: {
      ARGB32 prev_color = color;
      color.setColorGroup(value);
      if (color.v() != prev_color) {
        resetPixel();
        invalidate();
      }
    }
    break;
    case SCRIPTRECT_SETFILLED: {
      int was_filled = filled;
      filled = ATOI(value);
      if (was_filled != filled) invalidate();
    }
    break;
    case SCRIPTRECT_EDGES: {
      int prev_edges = edges;
      ParamParser pp(value);
      edges = 0;
      edges |= !!pp.hasString("left") * LEFT;
      edges |= !!pp.hasString("right") * RIGHT;
      edges |= !!pp.hasString("top") * TOP;
      edges |= !!pp.hasString("bottom") * BOTTOM;
      if (edges != prev_edges) invalidate();
    }
    break;
    case SCRIPTRECT_THICKNESS: {
      int prev_thickness = thickness;
      thickness = ATOI(value);
      if (thickness < 1) thickness = 1;
      if (thickness != prev_thickness) invalidate();
    }
    break;
    default:
      return 0;
  }
  return 1;
}

void ScriptRect::resetPixel() {
  pixel->fillBits(0xFF000000 | RGBTOBGR(color.v()));
}
