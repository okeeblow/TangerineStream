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
#include "../studio/api.h"
#include "cursor.h"

#define CBCLASS CursorI
START_DISPATCH;
  CB(CURSOR_GETOSHANDLE, getOSHandle);
END_DISPATCH;

#ifdef WASABI_COMPILE_SKIN

SkinCursor::SkinCursor(const char *elementid) {
  name = elementid;
  cursor = NULL;
  WASABI_API_SYSCB->syscb_registerCallback(static_cast<SkinCallbackI *>(this));
}

SkinCursor::SkinCursor() {
  WASABI_API_SYSCB->syscb_registerCallback(static_cast<SkinCallbackI *>(this));
  cursor = NULL;
}

SkinCursor::~SkinCursor() {
  WASABI_API_SYSCB->syscb_deregisterCallback(static_cast<SkinCallbackI *>(this));
}

OSCURSORHANDLE SkinCursor::getOSHandle() {
  if (cursor == NULL && !name.isempty()) {
    cursor = WASABI_API_SKIN->cursor_request(name);
  }
  return cursor;
}                  

int SkinCursor::skincb_onReset() {
  reset();
  return 1;
}

void SkinCursor::reset() {
  cursor = NULL;
}

void SkinCursor::setCursorElementId(const char *id) {
  name = id;
  reset();
}

#endif
