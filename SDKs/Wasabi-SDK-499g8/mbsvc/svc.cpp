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
#include <direct.h>
#include "mbsvc.h"
#include "svc.h"
#include "../studio/api.h"

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(MbSvc_Svc);
DECLARE_SERVICETSINGLE(svc_miniBrowser, MbSvc);
END_SERVICES(MbSvc_Svc, _MbSvc_Svc);

#ifdef _X86_
extern "C" { int _link_MbSvc; }
#else
extern "C" { int __link_MbSvc; }
#endif

#endif


int MbSvc::testGuid(GUID g) {
  return (g == GUID_MINIBROWSER_ANY || g == GUID_MINIBROWSER_IEACTIVEX);
}

MiniBrowser *MbSvc::createMiniBrowser() {
  BrowserWnd *w = new BrowserWnd;
  browsers.addItem(w);
  return w;
}

void MbSvc::destroyMiniBrowser(MiniBrowser *b) {
  ASSERT(b != NULL);
  BrowserWnd *bw = static_cast<BrowserWnd *>(b->minibrowser_getRootWnd());
  ASSERT(bw != NULL);
  int i = browsers.searchItem(bw);
  if (i < 0) return;
  browsers.removeByPos(i);
  delete bw;
}
  
