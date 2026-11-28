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

//<?#include "<class data="implementationheader"/>"
#include "svcenum.h"
//?>

#include "../studio/api.h"
#include "../common/xlatstr.h"

#include "../studio/services/services.h"
#include "../studio/services/waservicefactory.h"

SvcEnum::SvcEnum() : type(WaSvc::NONE), factory(NULL) {
  reset();
}

void *SvcEnum::_getNext(int global_lock) {
  if (WASABI_API_SVC == NULL) return NULL;
  for (;;) {
    factory = WASABI_API_SVC->service_enumService(type, pos++);
    if (factory == NULL) return NULL;
    void *s = factory->getInterface(FALSE);// get but don't lock
    ASSERT(s != NULL);
    if (_testService(s)) {
      if (global_lock)
        WASABI_API_SVC->service_lock(factory, s);	// lock in sys tables
      return s;
    }
    factory->releaseInterface(s);
  }
}

void SvcEnum::reset() {
  pos = 0;
  factory = NULL;
}

int SvcEnum::release(void *ptr) {
  return WASABI_API_SVC->service_release(ptr);
}

waServiceFactory *SvcEnum::getLastFactory() {
  return factory;
}
