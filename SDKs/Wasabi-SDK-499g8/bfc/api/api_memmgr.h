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
#ifndef __API_MEMMGR_H
#define __API_MEMMGR_H

#include "../dispatch.h"

class api_memmgr : public Dispatchable {
  public:
    void *sysMalloc(int size);
    void sysFree(void *ptr);
    void *sysRealloc(void *ptr, int newsize);
    void sysMemChanged(void *ptr);

  enum {
    API_MEMMGR_SYSMALLOC = 0,
    API_MEMMGR_SYSFREE = 10,
    API_MEMMGR_SYSREALLOC = 20,
    API_MEMMGR_SYSMEMCHANGED = 30,
  };
};

inline void *api_memmgr::sysMalloc(int size) {
  return _call(API_MEMMGR_SYSMALLOC, (void *)NULL, size);
}

inline void api_memmgr::sysFree(void *ptr) {
  _voidcall(API_MEMMGR_SYSFREE, ptr);
}

inline void *api_memmgr::sysRealloc(void *ptr, int newsize) {
  return _call(API_MEMMGR_SYSREALLOC, (void *)NULL, ptr, newsize);
}

inline void api_memmgr::sysMemChanged(void *ptr) {
  _voidcall(API_MEMMGR_SYSMEMCHANGED, ptr);
}

class api_memmgrI : public api_memmgr {
  public:
    virtual void *sysMalloc(int size)=0;
    virtual void sysFree(void *ptr)=0;
    virtual void *sysRealloc(void *ptr, int newsize)=0;
    virtual void sysMemChanged(void *ptr)=0;

  protected:
    RECVS_DISPATCH;
};

// {000CF46E-4DF6-4a43-BBE7-40E7A3EA02ED}
static const GUID memMgrApiServiceGuid = 
{ 0xcf46e, 0x4df6, 0x4a43, { 0xbb, 0xe7, 0x40, 0xe7, 0xa3, 0xea, 0x2, 0xed } };

extern api_memmgr *memmgrApi;

#endif
