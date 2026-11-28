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
#include "../../std.h"
#include "../../region.h"

#include "../../canvas.h"
#include "../../basewnd.h"

#define GETOSHANDLE(x) (const_cast<Region *>(x)->getOSHandle())

#define CBCLASS RegionI
START_DISPATCH;
  CB(REGION_GETOSHANDLE, getOSHandle);
  CB(REGION_CLONE, clone);
  VCB(REGION_DISPOSECLONE, disposeClone);
  CB(REGION_PTINREGION, ptInRegion);
  VCB(REGION_OFFSET, offset);
  VCB(REGION_GETBOX, getBox);
  VCB(REGION_SUBTRACTRGN, subtractRgn);
  VCB(REGION_SUBTRACTRECT, subtractRect);
  VCB(REGION_ADDRECT, addRect);
  VCB(REGION_ADD, add);
  VCB(REGION_AND, andRgn);
  VCB(REGION_SETRECT, setRect);
  VCB(REGION_EMPTY, empty);
  CB(REGION_ISEMPTY, isEmpty);
  CB(REGION_EQUALS, equals);
  CB(REGION_ENCLOSED, enclosed);
  CB(REGION_INTERSECTRECT, intersectRect);
  CB(REGION_INTERSECTRGN, intersectRgn);
  CB(REGION_ISRECT, isRect);
  VCB(REGION_SCALE, scale);
  VCB(REGION_DEBUG, debug);
  CB(REGION_MAKEWNDREGION, makeWindowRegion);
  CB(REGION_GETNUMRECTS, getNumRects);
  CB(REGION_ENUMRECT, enumRect);
END_DISPATCH;
#undef CBCLASS

RegionI::RegionI() {
  hrgn = XCreateRegion();
  init();
}

RegionI::RegionI(const RECT *r) {
  XRectangle xr = { r->left, r->top, r->right - r->left, r->bottom - r->top };
  hrgn = XCreateRegion();
  XUnionRectWithRegion( &xr, hrgn, hrgn );
  init();
}

RegionI::RegionI(int l, int t, int r, int b) {
  XRectangle xr = { l, t, r - l, b - t };
  hrgn = XCreateRegion();
  XUnionRectWithRegion( &xr, hrgn, hrgn );
  init();
}

RegionI::RegionI(OSREGION r) {
  hrgn = XCreateRegion();
  XUnionRegion( hrgn, r, hrgn );
  init();
}

RegionI::RegionI(Canvas *c) {
  HDC dc = c->getHDC();
  hrgn = XCreateRegion();

  if ( dc->clip ) {
    XUnionRegion( hrgn, dc->clip, hrgn );
  } else {
    int w, h;
    c->getDim( &w, &h, NULL );
    XRectangle xr = { 0, 0, w, h };
    XUnionRectWithRegion( &xr, hrgn, hrgn );
  }
  init();
}

RegionI::~RegionI() {
  delete lastdebug;
  ASSERT(clonecount==0);
  if (hrgn) {
    XDestroyRegion( hrgn );
    hrgn = NULL;
  }
} 

void RegionI::init() {
  clonecount = 0;
  lastdebug = NULL;
}

Region *RegionI::clone() {
  Region *newregion = new RegionI(getOSHandle());
  clonecount++;
  return newregion;
}

void RegionI::disposeClone(Region *r) {
  RegionI *ri = static_cast<RegionI *>(r);
  delete ri; // todo: validate pointer before deleting
  clonecount--;
}

// returns a handle that SetWindowRgn understands (non portable). We should NOT delete this handle, windows will delete 
// it by itself upon setting a new region of destroying the window
OSREGION RegionI::makeWindowRegion() {
  OSREGION R = XCreateRegion();
  XUnionRegion( R, hrgn, R  );
  return R;
}

RegionI::RegionI(SkinBitmap *bitmap, RECT *r, int xoffset, int yoffset, BOOL inverted, int dothreshold, char threshold, int thinverse, int minalpha) {
  if (r)
    hrgn = alphaToRegionRect(bitmap, xoffset, yoffset, TRUE, r->left, r->top, r->right-r->left, r->bottom-r->top, inverted, dothreshold, threshold, thinverse, minalpha);
  else
    hrgn = alphaToRegionRect(bitmap, xoffset, yoffset, FALSE, 0, 0, 0, 0, inverted, dothreshold, threshold, thinverse, minalpha);
  init();
}

OSREGION RegionI::alphaToRegionRect(SkinBitmap *bitmap, int xoffset, int yoffset, BOOL portion, int _x, int _y, int _w, int _h, BOOL inverted, int dothreshold, unsigned char threshold, int thinverse, int minalpha) {

  OSREGION ret = XCreateRegion();
  XRectangle xr;
  int x, y;

  int bmWidth=bitmap->getWidth();
  int bmHeight=bitmap->getHeight();
  int fullw=bitmap->getFullWidth();

  void *pbits32=bitmap->getBits();
  if (!pbits32) return NULL;

  int x_end=(portion ? _w+_x : bmWidth);
  int y_end=(portion ? _h+_y : bmHeight);
  int x_start=(portion ? _x : 0);
  int y_start=(portion ? _y : 0);

  x_start += bitmap->getX();
  x_end += bitmap->getX();
  y_start += bitmap->getY();
  y_end += bitmap->getY();

  unsigned int iv=minalpha << 24;

  int shiftx = xoffset - bitmap->getX();
  int shifty = yoffset - bitmap->getY();

  int xo=bitmap->getX();
  int yo=bitmap->getY();
  for (y = y_start; y < y_end; y++) {
    // Scan each bitmap pixel from left to right
    unsigned int *lineptr=((unsigned int *)pbits32) + fullw * y;
    for (x = x_start; x < x_end; x++) {
      // Search for a continuous range of "non transparent pixels"
      int x0 = x;
      unsigned int *p = lineptr;
      if (dothreshold) {
        if (inverted) {
          if (thinverse) {
	    while (x < x_end) {
              unsigned int a=p[x];
	      if ((a&0xff000000) >= iv || 
                  (((((a & 0xFF) > threshold || ((a & 0xFF00) >> 8) > threshold || ((a & 0xFF0000) >> 16) > threshold)))))
		break;
	      x++;
	    }
          }
          else {
	    while (x < x_end) {
              unsigned int a=p[x];
	      if ((a&0xff000000) >= iv || 
                  (((((a & 0xFF) < threshold || ((a & 0xFF00) >> 8) < threshold || ((a & 0xFF0000) >> 16) < threshold)))))
		break;
	      x++;
	    }
          }
        } else {
          if (thinverse) {
	    while (x < x_end) {
              unsigned int a=p[x];
	      if ((a&0xff000000) < iv || 
                  (((((a & 0xFF) > threshold || ((a & 0xFF00) >> 8) > threshold || ((a & 0xFF0000) >> 16) > threshold)))))
		break;
	      x++;
	    }
          }
          else {
	    while (x < x_end) {
              unsigned int a=p[x];
	      if ((a&0xff000000) < iv || 
                  (((((a & 0xFF) < threshold || ((a & 0xFF00) >> 8) < threshold || ((a & 0xFF0000) >> 16) < threshold)))))
		break;
	      x++;
	    }
          }
        }
      }
      else {
        if (inverted) {
	  while (x < x_end) {
	    if ((p[x] & 0xFF000000) >= iv) break;
            x++;
          }
        } else {
	  while (x < x_end) {
	    if ((p[x] & 0xFF000000) < iv) break;
            x++;
          }
        }
      }
      
      if (x > x0) {
	xr.x = x0 + shiftx; xr.y = y + shifty;
	xr.width = x - x0; xr.height = 1;

	XUnionRectWithRegion( &xr, ret, ret );
      }
    }
  }
  
  return ret;
}

BOOL RegionI::ptInRegion(const POINT *pt) {
  ASSERT(hrgn != NULL);
  return XPointInRegion( hrgn, pt->x, pt->y );
}

void RegionI::offset(int x, int y) {
  ASSERT(hrgn != NULL);
  if (x == 0 && y == 0) return;
  XOffsetRegion( hrgn, x, y );
}

void RegionI::getBox(RECT *r) {
  ASSERT(r != NULL);
  XRectangle xr;

  XClipBox( hrgn, &xr );

  r->top = xr.y;
  r->left = xr.x;
  r->right = xr.x + xr.width;
  r->bottom = xr.y + xr.height;
}

OSREGION RegionI::getOSHandle() {
  return hrgn;
}

void RegionI::subtractRect(const RECT *r) {
  RegionI s(r);
  subtractRgn(&s);
}

void RegionI::subtractRgn(const Region *reg) {
  XSubtractRegion( hrgn, GETOSHANDLE(reg), hrgn );
}

void RegionI::andRgn(const Region *reg) {
  XIntersectRegion( hrgn, GETOSHANDLE( reg ), hrgn );
}

void RegionI::addRect(const RECT *r) {
  RegionI a(r);
  add(&a);
}

void RegionI::add(const Region *reg) {
  ASSERT(reg != NULL);
  XUnionRegion( hrgn, GETOSHANDLE(reg), hrgn );
}

int RegionI::isEmpty() {
  return XEmptyRegion( hrgn );
}

int RegionI::enclosed(const Region *r, Region *outside) {
  OSREGION del=NULL;
  if (!outside) 
    del = XCreateRegion();
  else
    del = outside->getOSHandle();
  XSubtractRegion( hrgn, GETOSHANDLE( r ), del );
  int rs = XEmptyRegion( del );
  if (outside == NULL) XDestroyRegion(del);
  return rs;
}

int RegionI::intersectRgn(const Region *r, Region *intersection) {
  OSREGION del = NULL;
  if (intersection == NULL)
    del = XCreateRegion();
  else
    del=intersection->getOSHandle();

  XIntersectRegion( hrgn, GETOSHANDLE( r ), del );
  int rs = !XEmptyRegion( del );
  if (intersection == NULL) XDestroyRegion( del );
  return rs;
}

int RegionI::intersectRect(const RECT *r, Region *intersection) {
  RegionI rect(r);
  return intersectRgn( &rect, intersection );
}

void RegionI::empty() {
  ASSERT(hrgn != NULL);
  XDestroyRegion( hrgn );
  hrgn = XCreateRegion();
  ASSERT(hrgn != NULL);
}

void RegionI::setRect(const RECT *r) {
  ASSERT(hrgn != NULL);
  XDestroyRegion( hrgn );

  XRectangle xr = { r->left, r->top, r->right - r->left, r->bottom - r->top };
  hrgn = XCreateRegion();
  XUnionRectWithRegion( &xr, hrgn, hrgn );
}

int RegionI::equals(const Region *r) {
  ASSERT(r);
  return XEqualRegion( hrgn, GETOSHANDLE( r ) );
}

int RegionI::isRect() {
  RECT r;
  getBox( &r );

  return XRectInRegion( hrgn, r.left, r.top, r.right - r.left, r.bottom - r.top ) == RectangleIn;
}

void do_scale( HRGN inrgn, HRGN outrgn, XRectangle inrect, XRectangle outrect ) {
  switch( XRectInRegion( inrgn, inrect.x, inrect.y, inrect.width, inrect.height ) ) {
  case RectangleIn:
    XUnionRectWithRegion( &outrect, outrgn, outrgn );
    break;

  case RectangleOut:
    // Nothing to do..
    break;

  case RectanglePart:
    {
      XRectangle ti, to;
      ti = inrect; to = outrect;
      ti.width /= 2; ti.height /= 2;
      to.width /= 2; to.height /= 2;
      do_scale( inrgn, outrgn, ti, to );

      ti.x += ti.width;
      ti.width = inrect.width - ti.width;
      to.x += to.width;
      to.width = outrect.width - to.width;
      do_scale( inrgn, outrgn, ti, to );

      ti.x = inrect.x;
      ti.width = inrect.width / 2;
      ti.y += ti.height;
      ti.height = inrect.height - ti.height;
      to.x = outrect.x;
      to.width = outrect.width / 2;
      to.y += to.height;
      to.height = outrect.height - to.height;
      do_scale( inrgn, outrgn, ti, to );

      ti.x += ti.width;
      ti.width = inrect.width - ti.width;
      to.x += to.width;
      to.width = outrect.width - to.width;
      do_scale( inrgn, outrgn, ti, to );
    }
    break;
  }
}

void RegionI::scale(double sx, double sy, BOOL round) {
  HRGN outrgn = XCreateRegion();
  
  XRectangle inrect, outrect;
  XClipBox( hrgn, &inrect );
  outrect.x = inrect.x * sx;
  outrect.y = inrect.y * sy;
  outrect.width = inrect.width * sx;
  outrect.height = inrect.height * sy;

  do_scale( hrgn, outrgn, inrect, outrect );

  XDestroyRegion( hrgn );
  hrgn = outrgn;
}

void RegionI::debug(int async) {
  OutputDebugString( "portme -- RegionI::debug\n" );
}

// Sssh - we're not supposed to know about this...
struct BOX {
  short x1, x2, y1, y2;
};

struct internal_region {
  long size;
  long numRects;
  BOX *rects;
  BOX extents;
};

// later we can cache this data or something if needed
int RegionI::getNumRects() {
  return ((internal_region *)hrgn)->numRects;
}

int RegionI::enumRect(int n, RECT *r) {
  if ( n >= getNumRects() || n < 0 )
    return 0;

  BOX b = ((internal_region *)hrgn)->rects[n];

  r->left = b.x1; r->right = b.x2;
  r->top = b.y1; r->bottom = b.y2;
  return 1;
}
