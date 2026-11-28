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
//NONPORTABLE
#include <windows.h>

#include "event.h"

#include "../studio/assert.h"

WaitableEvent::WaitableEvent(char *name) {
  handle = CreateEvent(NULL, FALSE, FALSE, name);
  ASSERT(handle != NULL);	// should be an exception
}

WaitableEvent::~WaitableEvent() {
  CloseHandle(handle);
}

int WaitableEvent::wait(int timeout) {
  ASSERT(timeout == EVENT_WAIT_FOREVER || timeout >= 0);
  switch (WaitForSingleObject(handle, timeout)) {
    case WAIT_ABANDONED:
    case WAIT_TIMEOUT:
      return 0;
    case WAIT_OBJECT_0:
      return 1;
  }
  return -1;	// other error
}

int WaitableEvent::pulse() {
  return PulseEvent(handle);
}
