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
#include "xuigrid.h"
#include "../bfc/canvas.h"

// -----------------------------------------------------------------------

char GridXuiObjectStr[] = "Grid"; // xml tag
char GridXuiSvcName[] = "Grid XuiObject Service"; 

// -----------------------------------------------------------------------
Grid::Grid() {
  setRectRgn(1);
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "topleft",      GRID_SETTOPLEFT,      XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "top",          GRID_SETTOP,          XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "topright",     GRID_SETTOPRIGHT,     XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "left",         GRID_SETLEFT,         XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "middle",       GRID_SETMIDDLE,       XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "right",        GRID_SETRIGHT,        XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "bottomleft",   GRID_SETBOTTOMLEFT,   XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "bottom",       GRID_SETBOTTOM,       XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "bottomright",  GRID_SETBOTTOMRIGHT,  XUI_ATTRIBUTE_IMPLIED);
}

// -----------------------------------------------------------------------
Grid::~Grid() {
}

// -----------------------------------------------------------------------
int Grid::onInit() {
  GRID_PARENT::onInit();
  doPaint(NULL, 1); // computes the region
  invalidateWindowRegion();
  return 1;
}

// -----------------------------------------------------------------------
int Grid::onResize() {
  GRID_PARENT::onResize();
  doPaint(NULL, 1);
  invalidateWindowRegion();
  return 1;
}

// -----------------------------------------------------------------------
int Grid::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return GRID_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case GRID_SETTOPLEFT:
    case GRID_SETTOP:
    case GRID_SETTOPRIGHT:
    case GRID_SETLEFT:
    case GRID_SETMIDDLE:
    case GRID_SETRIGHT:
    case GRID_SETBOTTOMLEFT:
    case GRID_SETBOTTOM:
    case GRID_SETBOTTOMRIGHT:
      setGridImage(value, xmlattributeid);
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void Grid::setGridImage(const char *elementname, int what) {
  switch (what) {
    case GRID_SETTOPLEFT:       topleft = elementname;      break;
    case GRID_SETTOP:           top = elementname;          break;
    case GRID_SETTOPRIGHT:      topright = elementname;     break;
    case GRID_SETLEFT:          left = elementname;         break;
    case GRID_SETMIDDLE:        middle = elementname;       break;
    case GRID_SETRIGHT:         right = elementname;        break;
    case GRID_SETBOTTOMLEFT:    bottomleft = elementname;   break;
    case GRID_SETBOTTOM:        bottom = elementname;       break;
    case GRID_SETBOTTOMRIGHT:   bottomright = elementname;  break;
    default: return;
  }
  if (isInited()) invalidate(); 
}

// -----------------------------------------------------------------------
int Grid::onPaint(Canvas *canvas) {
  GRID_PARENT::onPaint(canvas);

  doPaint(canvas, 0);

  return 1;
}

void Grid::doPaint(Canvas *canvas, int dorgn) {

  RECT r;
  getGridRect(&r);

  int left_w = left.getBitmap() ? left.getBitmap()->getWidth() : 0;
  int left_h = left.getBitmap() ? left.getBitmap()->getHeight() : 0;
  int top_h = top.getBitmap() ? top.getBitmap()->getHeight() : 0;
  int top_w = top.getBitmap() ? top.getBitmap()->getWidth() : 0;
  int topleft_h = topleft.getBitmap() ? topleft.getBitmap()->getHeight() : 0;
  int topright_h = topright.getBitmap() ? topright.getBitmap()->getHeight() : 0;
  int topleft_w = topleft.getBitmap() ? topleft.getBitmap()->getWidth() : 0;
  int topright_w = topright.getBitmap() ? topright.getBitmap()->getWidth() : 0;
  int right_w = right.getBitmap() ? right.getBitmap()->getWidth() : 0;
  int right_h = right.getBitmap() ? right.getBitmap()->getHeight() : 0;
  int bottom_h = bottom.getBitmap() ? bottom.getBitmap()->getHeight() : 0;
  int bottom_w = bottom.getBitmap() ? bottom.getBitmap()->getWidth() : 0;
  int bottomleft_h = bottomleft.getBitmap() ? bottom.getBitmap()->getHeight() : 0;
  int bottomright_h = bottomright.getBitmap() ? bottom.getBitmap()->getHeight() : 0;
  int bottomleft_w = bottomleft.getBitmap() ? bottomleft.getBitmap()->getWidth() : 0;
  int bottomright_w = bottomright.getBitmap() ? bottomright.getBitmap()->getWidth() : 0;
  int middle_w = middle.getBitmap() ? middle.getBitmap()->getWidth() : 0;
  int middle_h = middle.getBitmap() ? middle.getBitmap()->getHeight() : 0;

  RECT cell;

  if (dorgn) reg.empty();

  // topleft
  cell.left = r.left;
  cell.top = r.top;
  cell.right = cell.left + topleft_w;
  cell.bottom = r.top + topleft_h;

  if (topleft.getBitmap()) {
    if (canvas) topleft.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn) {
      RegionI _r(topleft);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }
  
  // top
  cell.left = cell.right;
  cell.top = r.top;
  cell.right = r.right - topright_w;
  cell.bottom = r.top + top_h;

  if (top.getBitmap()) {
    if (canvas) top.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn && cell.left != cell.right) {
      RegionI _r(top);
      _r.scale((double)(cell.right-cell.left) / top_w, 1);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }
  
  // topright
  cell.left = cell.right;
  cell.top = r.top;
  cell.right = r.right;
  cell.bottom = r.top + topright_h;

  if (topright.getBitmap()) {
    if (canvas) topright.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn) {
      RegionI _r(topright);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }
  
  // left
  cell.left = r.left;
  cell.top = r.top + topleft_h;
  cell.right = r.left + left_w;
  cell.bottom = r.bottom - bottomleft_h;

  if (left.getBitmap()) {
    if (canvas) left.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn && cell.bottom != cell.top) {
      RegionI _r(left);
      _r.scale(1, (double)(cell.bottom-cell.top) / left_h);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }
  
  // middle
  cell.left = r.left + left_w;
  cell.top = r.top + top_h;
  cell.right = r.right - right_w;
  cell.bottom = r.bottom - bottom_h;

  if (middle.getBitmap()) {
    if (canvas) middle.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn && cell.left != cell.right && cell.bottom != cell.top) {
      RegionI _r(middle);
      _r.scale((double)(cell.right-cell.left) / middle_w, (double)(cell.bottom-cell.top) / middle_h);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }
  
  // right
  cell.left = r.right - right_w;
  cell.top = r.top + top_h;
  cell.right = r.right;
  cell.bottom = r.bottom - bottomright_h;

  if (right.getBitmap()) {
    if (canvas) right.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn && cell.bottom != cell.top) {
      RegionI _r(right);
      _r.scale(1, (double)(cell.bottom-cell.top) / right_h);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }

  // bottomleft
  cell.left = r.left;
  cell.top = r.bottom - bottomleft_h;
  cell.right = r.left + bottomleft_w;
  cell.bottom = r.bottom;

  if (bottomleft.getBitmap()) {
    if (canvas) bottomleft.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn) {
      RegionI _r(bottomleft);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }
  
  // bottom
  cell.left = cell.right;
  cell.top = r.bottom - bottom_h;
  cell.right = r.right - bottomright_w;
  cell.bottom = r.bottom;

  if (bottom.getBitmap()) {
    if (canvas) bottom.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn && cell.right != cell.left) {
      RegionI _r(bottom);
      _r.scale((double)(cell.right-cell.left) / bottom_w, 1);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }

  // bottomright
  cell.left = cell.right;
  cell.top = r.bottom - bottomright_h;
  cell.right = r.right;
  cell.bottom = r.bottom;

  if (bottomright.getBitmap()) {
    if (canvas) bottomright.getBitmap()->stretchToRectAlpha(canvas, &cell, getPaintingAlpha());
    if (dorgn) {
      RegionI _r(bottomright);
      _r.offset(cell.left-r.left, cell.top-r.top);
      reg.addRegion(&_r);
    }
  }
}