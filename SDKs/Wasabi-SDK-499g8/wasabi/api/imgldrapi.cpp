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
#include "imgldrapi.h"
#include "../../studio/imgldr.h"
#ifdef WASABI_COMPILE_SKIN
#include "../../studio/skinelem.h"
#endif
imgldr_api *imgLoaderApi = NULL;

ARGB32 *ImgLdrApi::imgldr_makeBmp(const char *filename, int *has_alpha, int *w, int *h)
{
  if (filename == NULL) {
    DebugString("illegal param : filename == NULL");
    return NULL;
  }
  return imageLoader::makeBmp(filename, NULL, has_alpha, w, h, NULL, TRUE, NULL);
}

ARGB32 *ImgLdrApi::imgldr_makeBmp2(HINSTANCE hInst, int id, int *has_alpha, int *w, int *h, const char *colorgroup) {
  return imageLoader::makeBmp(hInst, id, has_alpha,w,h, colorgroup);
}

void ImgLdrApi::imgldr_releaseBmp(ARGB32 *bmpbits) {
  if (bmpbits == NULL) {
    DebugString("illegal param : bmpbits == NULL");
    return;
  }
  imageLoader::release(bmpbits);
}

#ifdef WASABI_COMPILE_SKIN

ARGB32 *ImgLdrApi::imgldr_requestSkinBitmap(const char *file, int *has_alpha, int *x, int *y, int *subw, int *subh, int *w, int *h, int cached) {
  if (file == NULL) {
    DebugString("illegal param : file == NULL");
    return NULL;
  }
  return imageLoader::requestSkinBitmap(file, has_alpha, x, y, subw, subh, w, h, cached);
}

void ImgLdrApi::imgldr_releaseSkinBitmap(ARGB32 *bmpbits) {
  if (bmpbits == NULL) {
    DebugString("illegal param : bmpbits == NULL");
    return;
  }
  imageLoader::releaseSkinBitmap(bmpbits);
}

RegionServer *ImgLdrApi::imgldr_requestSkinRegion(const char *id) {
  if (id == NULL) {
    DebugString("illegal param : id == NULL");
    return NULL;
  }
  return SkinElementsMgr::requestSkinRegion(id);
}

void ImgLdrApi::imgldr_cacheSkinRegion(const char *id, Region *r) {
  if (id == NULL) {
    DebugString("illegal param : id == NULL");
  }
  if (r == NULL) {
    DebugString("illegal param : region == NULL");
  }
  SkinElementsMgr::cacheSkinRegion(id, r);
}

#endif

