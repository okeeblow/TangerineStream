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
#ifndef __API_LOCALESI_IMPL_H
#define __API_LOCALESI_IMPL_H

/*<?<autoheader/>*/
#include "api_locales.h"
#include "api_localesx.h"
/*?>*/

class api_localesI : public api_localesX {
public:
  api_localesI();
  virtual ~api_localesI();
  
  DISPATCH(10) const char *locales_getTranslation(const char *str); // if not found, returns the str paramer
  DISPATCH(20) void locales_addTranslation(const char *from, const char *to);
  DISPATCH(30) const char *locales_getBindFromAction(int action);
  DISPATCH(40) int locales_getNumEntries();
  DISPATCH(50) const char *locales_enumEntry(int n);
  DISPATCH(60) void locales_registerAcceleratorSection(const char *name, RootWnd *wnd, int global = 0);
};


#endif // __API_LOCALESI_IMPL_H
