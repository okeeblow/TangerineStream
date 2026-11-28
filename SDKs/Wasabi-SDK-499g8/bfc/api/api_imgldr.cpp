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
#include "api_imgldr.h"

#ifdef CBCLASS
#undef CBCLASS
#endif
#define CBCLASS imgldr_apiI
START_DISPATCH;
  CB(IMGLDR_API_MAKEBMP,            imgldr_makeBmp);
  CB(IMGLDR_API_MAKEBMP2,           imgldr_makeBmp2);
  VCB(IMGLDR_API_RELEASEBMP,        imgldr_releaseBmp);
#ifdef WASABI_COMPILE_SKIN
  CB(IMGLDR_API_REQUESTSKINBITMAP,  imgldr_requestSkinBitmap);
  CB(IMGLDR_API_REQUESTSKINREGION,  imgldr_requestSkinRegion);
  VCB(IMGLDR_API_CACHESKINREGION,   imgldr_cacheSkinRegion);
  VCB(IMGLDR_API_RELEASESKINBITMAP, imgldr_releaseSkinBitmap);
#endif //WASABI_COMPILE_SKIN
END_DISPATCH;
