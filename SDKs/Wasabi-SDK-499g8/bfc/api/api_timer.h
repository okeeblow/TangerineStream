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
#ifndef __TIMER_API_H
#define __TIMER_API_H

#include "../dispatch.h"
#include "../platform/platform.h"

class TimerClient;

class timer_api : public Dispatchable {
  public:
    void timer_add(TimerClient *client, int id, int ms);
    void timer_remove(TimerClient *client, int id = -1);

  enum {
    TIMER_API_ADD = 0,
    TIMER_API_REMOVE = 10,
  };
};

inline void timer_api::timer_add(TimerClient *client, int id, int ms) {
  _voidcall(TIMER_API_ADD, client, id, ms);
}

inline void timer_api::timer_remove(TimerClient *client, int id) {
  _voidcall(TIMER_API_REMOVE, client, id);
}

class timer_apiI : public timer_api {
  public:
    virtual void timer_add(TimerClient *client, int id, int ms)=0;
    virtual void timer_remove(TimerClient *client, int id = -1)=0;

  protected:
    RECVS_DISPATCH;
};

// {3130D81C-AE1F-4954-9765-698473B627B0}
static const GUID timerApiServiceGuid = 
{ 0x3130d81c, 0xae1f, 0x4954, { 0x97, 0x65, 0x69, 0x84, 0x73, 0xb6, 0x27, 0xb0 } };

extern timer_api *timerApi;

#endif