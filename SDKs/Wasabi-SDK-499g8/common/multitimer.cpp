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
#include "multitimer.h"
#include "../studio/api.h"

MultiTimer::MultiTimer() {
  hi = 25555555;
  setParent(api->main_getRootWnd());
  setStartHidden(1);
  init(api->main_getRootWnd(), TRUE);
}

MultiTimer::~MultiTimer() {
}

int MultiTimer::addTimer(int delay, userTimerProc p, void *d1, void *d2) {
  userTimerProcS *utps = new userTimerProcS;
  utps->fn = p;
  utps->data1 = d1;
  utps->data2 = d2;
  utps->id = hi++;
  ASSERTPR(hi != 0, "exhausted timer ids (dude, that's a lot)");
  setTimer(utps->id, delay);
  timerprocs.addItem(utps);
  return utps->id;
}

void MultiTimer::delTimer(int id) {
  for (int i=0;i<timerprocs.getNumItems();i++) {
    userTimerProcS *s = timerprocs.enumItem(i);
    if (s->id == id) {
      killTimer(s->id);
      timerprocs.removeByPos(i);
      delete s;
      return;
    }
  }
}

void MultiTimer::timerCallback(int id) {
  if (id < 25555555) {
    MULTITIMER_PARENT::timerCallback(id);
    return;
  }
  for (int i=0;i<timerprocs.getNumItems();i++) {
    userTimerProcS *s = timerprocs.enumItem(i);
    if (s->id == id) {
      s->fn(s->id, s->data1, s->data2);
      return;
    }
  }
  MULTITIMER_PARENT::timerCallback(id);
}

