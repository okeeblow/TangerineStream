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
#ifndef __IMGLDRAPI_H
#define __IMGLDRAPI_H

#include "../../bfc/api/api_imgldr.h"

class ImgLdrApi : public imgldr_apiI {
  public:
     ARGB32 *imgldr_makeBmp(const char *filename, int *has_alpha, int *w, int *h);
     ARGB32 *imgldr_makeBmp2(HINSTANCE hInst, int id, int *has_alpha, int *w, int *h, const char *colorgroup = NULL);
     void imgldr_releaseBmp(ARGB32 *bmpbits);
#ifdef WASABI_COMPILE_SKIN
     ARGB32 *imgldr_requestSkinBitmap(const char *file, int *has_alpha, int *x, int *y, int *subw, int *subh, int *w, int *h, int cached);
     RegionServer *imgldr_requestSkinRegion(const char *id);
     void imgldr_cacheSkinRegion(const char *id, Region *r);
     void imgldr_releaseSkinBitmap(ARGB32 *bmpbits);
#endif //WASABI_COMPILE_SKIN
};


#endif
