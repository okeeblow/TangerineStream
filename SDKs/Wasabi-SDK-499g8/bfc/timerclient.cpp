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
#include "api/api_timer.h"
#include "timerclient.h"
#include "../studio/api.h"

#define CBCLASS TimerClientI
START_DISPATCH;
  // this one doesn't map directly so that we can catch deferredcb timers
  VCB(TIMERCLIENT_TIMERCALLBACK,   timerclient_handleDeferredCallback);
  CB(TIMERCLIENT_GETMASTERCLIENT,  timerclient_getMasterClient);
  VCB(TIMERCLIENT_ONMASTERMUX,     timerclient_onMasterClientMultiplex);
  CB(TIMERCLIENT_GETDEPPTR,        timerclient_getDependencyPtr);
  CB(TIMERCLIENT_GETSKIPPED,       timerclient_getSkipped);
  VCB(TIMERCLIENT_SETSKIPPED,      timerclient_setSkipped);
  VCB(TIMERCLIENT_POSTDEFERREDCB , timerclient_postDeferredCallback);
  CB(TIMERCLIENT_ONDEFERREDCB ,    timerclient_onDeferredCallback);
  CB(TIMERCLIENT_GETNAME,          timerclient_getName);
END_DISPATCH;

TimerClientI::TimerClientI() :
  skipped(0), timerdelay(0), disallowset(0)
{ }

TimerClientI::~TimerClientI() {
  disallowset = 1;
  if (cbs.getNumItems() > 0) 
    WASABI_API_TIMER->timer_remove(this, DEFERREDCB_TIMERID);
  cbs.deleteAll();
}

int TimerClientI::timerclient_setTimer(int id, int ms) {
  if (disallowset) return 0;
  ASSERTPR(id > 0, "A timer id cannot be <= 0");
  ASSERTPR(id != DEFERREDCB_TIMERID, "please chose another timer id");
  WASABI_API_TIMER->timer_add(this, id, ms);
  return 1;
}

int TimerClientI::timerclient_killTimer(int id) {
  WASABI_API_TIMER->timer_remove(this, id);
  return 1;
}

void TimerClientI::timerclient_postDeferredCallback(int param1, int param2, int mindelay) {
  if (disallowset) return;
  for (int i=0;i<cbs.getNumItems();i++) {
    deferred_callback *cb = cbs.enumItem(i);
    if (cb->param1 == param1 && cb->param2 == param2) {
      cbs.removeByPos(i);
      break;
    }
  }
  deferred_callback *c = new deferred_callback;
  c->origin = this;
  c->param1 = param1;
  c->param2 = param2;
  cbs.addItem(c);
  WASABI_API_TIMER->timer_add(this, DEFERREDCB_TIMERID, MAX(1, mindelay));
}

void TimerClientI::timerclient_handleDeferredCallback(int id) {

  // let deriving class handle it. note we call into here even if
  // it's the deferred cb id, since older versions did that too,
  // expecting the user to call down on the method.
  timerclient_timerCallback(id);

  // process our deferred cb id
  if (id == DEFERREDCB_TIMERID) {
    WASABI_API_TIMER->timer_remove(this, DEFERREDCB_TIMERID);
    PtrList<deferred_callback> temp(cbs);
    cbs.removeAll();
    foreach(temp)
      deferred_callback *c = temp.getfor();
      c->origin->timerclient_onDeferredCallback(c->param1, c->param2);
      delete c;
    endfor
  }
}
