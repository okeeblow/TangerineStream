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
#ifndef _SVCENUMT_H
#define _SVCENUMT_H

#include "svcenum.h"

template <class T>
class COMEXP SvcEnumT : private SvcEnum {
protected:
  SvcEnumT() { type = T::getServiceType(); }

public:
  void reset() { SvcEnum::reset(); }
  T *getFirst(int global_lock = TRUE) { reset(); return getNext(global_lock); }
  T *getNext(int global_lock = TRUE) { return static_cast<T*>(_getNext(global_lock)); }

  // these would just be 'using' but msvc.net sucks butt
  inline int release(void *ptr) { return SvcEnum::release(ptr); }
  inline waServiceFactory *getLastFactory() { return SvcEnum::getLastFactory(); }

protected:
  // override this one (or don't if you want to return all of them)
  virtual int testService(T *svc) { return TRUE; }

private:
  virtual int _testService(void *svc) {
    return testService(static_cast<T *>(svc));
  }
};

#endif // _SVCENUMT_H
