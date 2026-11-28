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
#ifndef _SVC_ENUM_H
#define _SVC_ENUM_H

/*#include "../common/common.h"

#include "memblock.h"
#include "ptrlist.h"

#include "../studio/api.h"
#include "../common/metatags.h"

#include "../studio/services/service.h"*/

// try to use this one, it does a clientLock for you
template <class T>
class castService {
public:
  castService(waServiceFactory *w, int global_lock=TRUE) : was(w), gl(global_lock) { }
  operator T *() {
    if (was == NULL) return NULL;
    T *ret = static_cast<T *>(was->getInterface(gl));
    return ret;
  }
private:
  waServiceFactory *was;
  int gl;
};

#include "svcenumbyguid.h"
#include "svcenum.h"
#include "svcenumt.h"

#endif
