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

#include "thread.h"

#if !defined(WIN32) && !defined(LINUX)
#error port me!
#endif

THREADCALL Thread::ThreadProc(void *param) {
  friend class Thread;
  Thread *th = static_cast<Thread*>(param);
  th->isrunning = 1;
  int ret = th->threadProc();
  th->isrunning = 0;
  return ret;
}

Thread::Thread(HANDLE _parent_handle) :
  isrunning(0),
  killswitch(0),
  handle(NULL), parent_handle(NULL),
  auto_close_parent(FALSE)
{
  setParentHandle(_parent_handle);
#ifdef WIN32
  handle = CreateThread(NULL, 0, ThreadProc, (LPVOID)this, CREATE_SUSPENDED, &threadid);
#endif
}

Thread::~Thread() {
  setKillSwitch(TRUE);
  end();
  // FUCKO: might need to get rough here
#ifdef WIN32
  CloseHandle(handle);
#endif
  setParentHandle(NULL);
}

int Thread::start() {
  if (running()) return 1;
#ifdef WIN32
  return !(ResumeThread(handle) == 0xffffffff);
#endif
#ifdef LINUX
  pthread_create(&handle, NULL, ThreadProc, (LPVOID)this);
  return 1;
#endif
}

BOOL Thread::running() {
  return isrunning;
}

int Thread::end() {
  while (running()) Sleep(66);
  return 1;
}

void Thread::setKillSwitch(int k) {
  killswitch = k;
}

int Thread::getKillSwitch() {
  if (killswitch) return 1;
  if (parent_handle != NULL && !parentRunning()) return 1;
  return 0;
}

THREADID Thread::getThreadId() const {
  return threadid;
}

void Thread::setParentHandle(HANDLE _parent_handle, int _auto_close) {
  if (parent_handle != NULL && auto_close_parent) {
#ifdef WIN32
    CloseHandle(parent_handle);
#else
#error port me!
#endif
  }
  parent_handle = _parent_handle;
  auto_close_parent = _auto_close;
}

int Thread::parentRunning() {
  if (parent_handle == NULL) return -1;
#ifdef WIN32
  DWORD exitcode;
  int r = GetExitCodeThread(parent_handle, &exitcode);
  if (r == 0 || exitcode != STILL_ACTIVE) return 0;
  return 1;
#else
#error port me!
#endif
}

void Thread::setPriority(int priority) {
  Std::setThreadPriority(priority, handle);
}
