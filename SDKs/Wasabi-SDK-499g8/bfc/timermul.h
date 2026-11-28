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
#ifndef __TIMER_MULTIPLEXER_H
#define __TIMER_MULTIPLEXER_H

#include "../common/common.h"
#include "ptrlist.h"

class CfgItem;

class COMEXP TimerMultiplexerClient {
  public:
    virtual void onMultiplexedTimer(void *data, int skip, int mssincelast)=0;  
};

class MultiplexedTimer {
  public:
    MultiplexedTimer(int _ms, void *_data) : ms(_ms), data(_data) { 
      nexttick=0;
      flag=0;
      lost = 0; 
      lastmscount=0; 
      lastdelay=0;
    }
    virtual ~MultiplexedTimer() { }

    int ms;
    void *data;
    DWORD nexttick; // only used by low precision timers
    int flag; // only used by hi precision timers
    float lost; // only used by hi precision timers
    DWORD lastmscount;
    int lastdelay;
};

class COMEXP TimerMultiplexer {
  public:

    TimerMultiplexer();
    virtual ~TimerMultiplexer();

    virtual void setClient(TimerMultiplexerClient *client);

    virtual void onServerTimer();

    virtual void addTimer(int ms, void *data);
    virtual void removeTimer(void *data);
    virtual void setResolution(int ms);

    virtual void shutdown();
    virtual int getNumTimers();
    virtual int getNumTimersLP();

  private:

    void checkResolution(DWORD now);
    void resetTimer(int newresolution);
    void resetWheel();
    void distributeAll();
    void distribute(MultiplexedTimer *t);
    void runCurSlice(DWORD now);
    void runTimer(DWORD now, DWORD last, MultiplexedTimer *t, PtrList<MultiplexedTimer> *slice, int pos);
    void removeFromWheel(MultiplexedTimer *t);
    void runLowPrecisionTimers(DWORD now);
    void removeFromLowPrecision(MultiplexedTimer *t);
    void doShutdown();
    PtrList<MultiplexedTimer> *getSlice(int n);
    
    TimerMultiplexerClient *client;    
    int resolution;
    DWORD last_resolution_check;
    int timerset;
    
    int curslice;
    int nslices;
    int justexited;
    int firstevent;

    PtrList< PtrList< MultiplexedTimer > > wheel;
    PtrList< MultiplexedTimer > timers;
    PtrList< MultiplexedTimer > lptimers;
    MultiplexedTimer *running_timer;

  CfgItem *uioptions;
};

class MultiplexerServer {
public:
  MultiplexerServer(TimerMultiplexer *mux, UINT tid) : m_mux(mux), m_tid(tid) {}
  virtual ~MultiplexerServer() {}
  TimerMultiplexer *getMultiplexer() { return m_mux; }
  UINT getId() { return m_tid; }
  void setId(UINT id) { m_tid = id; }
private:
  TimerMultiplexer *m_mux;
  UINT m_tid;
};

class COMEXP MultiplexerServerComparatorTID {
public:
  // comparator for sorting
  static int compareItem(MultiplexerServer *p1, MultiplexerServer* p2) {
    if (p1->getId() < p2->getId()) return -1;
    if (p1->getId() > p2->getId()) return 1;
    return 0;
  }
  // comparator for searching
  static int compareAttrib(const char *attrib, MultiplexerServer *item) {
    if (*((UINT *)attrib) < item->getId()) return -1;
    if (*((UINT *)attrib) < item->getId()) return 1;
    return 0;
  }
};

class COMEXP MultiplexerServerComparatorMux{
public:
  // comparator for sorting
  static int compareItem(MultiplexerServer *p1, MultiplexerServer* p2) {
    if (p1->getMultiplexer() < p2->getMultiplexer()) return -1;
    if (p1->getMultiplexer() > p2->getMultiplexer()) return 1;
    return 0;
  }
  // comparator for searching
  static int compareAttrib(const char *attrib, MultiplexerServer *item) {
    if ((TimerMultiplexer *)attrib < item->getMultiplexer()) return -1;
    if ((TimerMultiplexer *)attrib < item->getMultiplexer()) return 1;
    return 0;
  }
};

#endif
