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
#include "autobitmap.h"

#include "../studio/api.h"

#define TIMER_ID_RESET 0x1664

#ifdef DROP_BITMAP_ON_IDLE
// these are in seconds
#define DROP_MINDELAY 3
#define DROP_MAXDELAY 15
#define DROP_INITIALBUMP -5
#define DROP_MINDELAYSINCELASTUSE 7
#endif

AutoSkinBitmap::AutoSkinBitmap(const char *_name) {
  bitmap = NULL;
  use = 0;
  id = 0;
#ifdef WIN32
  myInstance = WASABI_API_APP->main_gethInstance();
#endif
  setBitmap(_name);
}

AutoSkinBitmap::~AutoSkinBitmap() {
#ifdef DROP_BITMAP_ON_IDLE
  timerclient_killTimer(TIMER_ID_RESET);
#endif
  delete bitmap;
#ifdef WASABI_COMPILE_SKIN
  ASSERT(WASABI_API_SYSCB != NULL);
  WASABI_API_SYSCB->syscb_deregisterCallback(this);
#endif
}

const char *AutoSkinBitmap::setBitmap(const char *_name) {
  if (_name == NULL) return NULL;
  if (name == NULL || !STREQL(name, _name)) {
    reset();
    name = _name;
  }
  return name;
}

int AutoSkinBitmap::setBitmap(int _id) {
  if (_id == 0) return 0;
  if (_id != id) {
    reset();
    id = _id;
  }
  return id;
}

void AutoSkinBitmap::setHInstance(HINSTANCE hinstance) {
  myInstance = hinstance;
}

void AutoSkinBitmap::reset() {
  delete bitmap; bitmap = NULL;
#ifdef DROP_BITMAP_ON_IDLE
  timerclient_killTimer(TIMER_ID_RESET);
#endif
}

SkinBitmap *AutoSkinBitmap::getBitmap() {
//FG  ASSERT(name != NULL);
  if (name == NULL && id == NULL) return NULL;
  if (bitmap == NULL) {
    if (name) {
#ifdef WASABI_COMPILE_IMGLDR
      bitmap = new SkinBitmap(name);
#endif
    } else {
#ifdef WIN32                                    
#ifdef WASABI_COMPILE_IMGLDR
      bitmap = new SkinBitmap(myInstance, id, colorgroup);
#endif
#endif
    }
#ifdef WASABI_COMPILE_SKIN
    ASSERT(WASABI_API_SYSCB != NULL);
    if (bitmap)
      WASABI_API_SYSCB->syscb_registerCallback(this);
#endif
#ifdef DROP_BITMAP_ON_IDLE
    if (bitmap) {
      lastuse = Std::getTickCount() + DROP_INITIALBUMP; 
      timerclient_setTimer(TIMER_ID_RESET, DROP_MINDELAY*1000 + Std::random((DROP_MAXDELAY-DROP_MINDELAY)*1000));
      return bitmap;
    }
#endif
  }
#ifdef DROP_BITMAP_ON_IDLE
  if (bitmap) lastuse = Std::getTickCount();
#endif
  return bitmap;
}

const char *AutoSkinBitmap::getBitmapName() {
  return name;
}

int AutoSkinBitmap::skincb_onReset() {
  reset();
  return 1;
}

#ifdef DROP_BITMAP_ON_IDLE
void AutoSkinBitmap::timerclient_timerCallback(int id) {
  if (id == TIMER_ID_RESET) {
    tryUnload();
  } else TimerClientDI::timerclient_timerCallback(id);
}

void AutoSkinBitmap::tryUnload() {
  DWORD now = Std::getTickCount();
  if (now < lastuse + DROP_MINDELAYSINCELASTUSE*1000) return; 
  reset();
}
#endif
