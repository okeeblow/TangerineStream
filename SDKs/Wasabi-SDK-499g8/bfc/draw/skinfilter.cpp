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

#include "skinfilter.h"

#include <studio/services/svc_skinfilter.h>

void ApplySkinFilters::apply(const char *elementid, const char *forced_gammagroup, ARGB32 *bits, int w, int h, int bpp) {
  if ((elementid == NULL && forced_gammagroup == NULL) || bits == NULL || w <= 0 || h <= 0) return;
  SkinFilterEnum sfe;

  svc_skinFilter *obj;
  while (1) {
    obj = sfe.getNext(FALSE);
    if (!obj) break;
    obj->filterBitmap((unsigned char *)bits, w, h, bpp, elementid, forced_gammagroup);
    sfe.getLastFactory()->releaseInterface(obj);
  }
}
