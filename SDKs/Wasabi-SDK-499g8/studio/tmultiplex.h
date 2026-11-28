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
#ifndef __MAIN_TIMERMULTIPLEXER_H
#define __MAIN_TIMERMULTIPLEXER_H

#include "../bfc/timermul.h"
#include "../bfc/depend.h"
#include "../bfc/string.h"
#include "../bfc/timerclient.h"

class VirtualTimer {
  public:
    VirtualTimer(TimerClient *_client, int _id, Dependent *depend);
    virtual ~VirtualTimer() { }

    TimerClient *client, *mclient;
    Dependent *dep;
    String name;
    int id;
};

class MainTimerMultiplexer : public TimerMultiplexer, public TimerMultiplexerClient, public DependentViewerI  {
  public:

    MainTimerMultiplexer();
    virtual ~MainTimerMultiplexer();
    
    virtual void add(TimerClient *client, int id, int ms);
    virtual void remove(TimerClient *client, int id);
  
    virtual void onMultiplexedTimer(void *data, int skip, int mssincelast);
    virtual void onServerTimer();

    virtual int viewer_onItemDeleted(Dependent *item);

  private:

    int isValidTimerClientPtr(TimerClient *tc, Dependent *dep);

    int haveClient(TimerClient *client);
    PtrList<VirtualTimer> timerclients;
    PtrListQuickSortedByPtrVal<TimerClient> masters;
  
};

extern MainTimerMultiplexer *mainmultiplex;

#endif
