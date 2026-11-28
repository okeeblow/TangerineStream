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

#include "redirstr.h"

#include "../../studio/services/svc_redir.h"

#define MAX_REDIRECT 256

RedirString::RedirString(const char *val, const char *domain) {
  if (val == NULL) return;
  String fn = val;
  for (int done = 0, c = 0; !done && c < MAX_REDIRECT; done = 1, c++) {
    RedirectEnum re(fn);
    svc_redirect *svc;
    while ((svc = re.getNext(FALSE)) != NULL) {
      char buf[WA_MAX_PATH]="";
      if (svc->redirect(fn, "Filename", buf, sizeof(buf))) {
        fn = buf;
        done = 0;
      }
      re.getLastFactory()->releaseInterface(svc); svc = NULL;
    }
  }
  setValue(fn);
}
