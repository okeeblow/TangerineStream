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
#ifndef _SYSMEMCB_H
#define _SYSMEMCB_H

#include "syscb.h"

namespace SysMemCallback {
  enum {
    ONMALLOC=10,
    ONFREE=20,
    ONREALLOC=30,
    ONCHANGE=40,
  };
};

#define SYSMEMCALLBACK_PARENT SysCallbackI
class SysMemCallbackI : public SYSMEMCALLBACK_PARENT {
protected:
  SysMemCallbackI() { }

public:
  virtual void sysmem_onMalloc(void *memory, int size)=0;
  virtual void sysmem_onFree(void *memory)=0;
  virtual void sysmem_onRealloc(void *prev_memory, void *new_memory, int new_size)=0;
  virtual void sysmem_onChange(void *memory)=0;

private:
  virtual FOURCC syscb_getEventType() { return SysCallback::SYSMEM; }
  virtual int syscb_notify(int msg, int param1=0, int param2=0) {
    switch (msg) {
      case SysMemCallback::ONMALLOC:
         sysmem_onMalloc((void *)param1, param2);
      break;
      case SysMemCallback::ONFREE:
         sysmem_onFree((void *)param1);
      break;
      case SysMemCallback::ONREALLOC: {
         void **ptrs = (void **)param1;
         sysmem_onRealloc(ptrs[0], ptrs[1], param2);
      }
      break;
      case SysMemCallback::ONCHANGE:
         sysmem_onChange((void *)param1);
      break;
      default: return 0;
    }
    return 1;
  }
};

#endif
