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
#include "critsec.h"
#include "../studio/assert.h"

#if !defined(WIN32) && !defined(LINUX)
#error port me!
#endif

// uncomment this if needed
//#define CS_DEBUG

CriticalSection::CriticalSection() {
#ifdef WIN32
  InitializeCriticalSection(&cs);
#endif
#ifdef LINUX
  pthread_mutex_t recursive = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

  cs.mutex = recursive;
#endif

#ifdef ASSERTS_ENABLED
#ifdef CS_DEBUG
  within = 0;
#endif
#endif
}

CriticalSection::~CriticalSection() {
#ifdef CS_DEBUG
#ifdef ASSERTS_ENABLED
  ASSERT(!within);
#endif
#endif
#ifdef WIN32
  DeleteCriticalSection(&cs);
#endif
#ifdef LINUX
  pthread_mutex_destroy(&cs.mutex);
#endif
}

void CriticalSection::enter() {
#ifdef WIN32
  EnterCriticalSection(&cs);
#endif
#ifdef LINUX
  pthread_mutex_lock(&cs.mutex);
#endif

#ifdef CS_DEBUG
#ifdef ASSERTS_ENABLED
  ASSERT(!within);
  within = 1;
#endif
#endif
}

void CriticalSection::leave() {
#if defined(CS_DEBUG) && defined(ASSERTS_ENABLED)
  ASSERT(within);
  within = 0;
#endif

#ifdef WIN32
  LeaveCriticalSection(&cs);
#endif
#ifdef LINUX
  pthread_mutex_unlock(&cs.mutex);
#endif
}

void CriticalSection::inout() {
  enter();
  leave();
}
