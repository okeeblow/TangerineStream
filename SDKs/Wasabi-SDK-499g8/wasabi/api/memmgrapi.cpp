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
#include "memmgrapi.h"
#include "../../bfc/std.h"

#ifdef WASABI_COMPILE_SYSCB
  #include "../../studio/syscb.h"
  #include "../../studio/sysmemcb.h"
  #include "../../studio/cbmgr.h"
#endif

api_memmgr *memmgrApi = NULL;

void *MemMgrApi::sysMalloc(int size) {
  if (size <= 0) return NULL;
  void *ret = MALLOC(size);
#ifdef WASABI_COMPILE_SYSCB
  if (ret != NULL) CallbackManager::issueCallback(SysCallback::SYSMEM, SysMemCallback::ONMALLOC, (int)ret, size);
#endif
  return ret;
}

void MemMgrApi::sysFree(void *ptr) {
#ifdef WASABI_COMPILE_SYSCB
  if (ptr != NULL) CallbackManager::issueCallback(SysCallback::SYSMEM, SysMemCallback::ONFREE, (int)ptr);
#endif
  FREE(ptr);
}

void *MemMgrApi::sysRealloc(void *ptr, int newsize) {
  void *ptrs[2] = { ptr,  };
  void *new_memory = REALLOC(ptr, newsize);
  ptrs[1] = new_memory;
#ifdef WASABI_COMPILE_SYSCB
  if (!(ptrs[0] == NULL && ptrs[1] == NULL))
    CallbackManager::issueCallback(SysCallback::SYSMEM, SysMemCallback::ONREALLOC, (int)ptrs, newsize);
#endif
  return new_memory;
}

void MemMgrApi::sysMemChanged(void *ptr) {
  if (ptr == NULL) return;
  CallbackManager::issueCallback(SysCallback::SYSMEM, SysMemCallback::ONCHANGE,
(int)ptr);
}
