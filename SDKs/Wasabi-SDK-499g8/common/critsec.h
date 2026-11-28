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
//NONPORTABLE (api is portable, implementation is not)
#ifndef _CRITSEC_H
#define _CRITSEC_H

#ifdef WIN32
#include <windows.h>
#endif

#include "common.h"
#include "../studio/assert.h"

/**
  CriticalSection is a portable object that implements a critical section,
  which is to say, it ensures that no two threads can be in that same
  section of code at the same time. Usually you make them a global object
  or allocate them with new and pass them to both threads.

  @short Critical section class.
  @author Nullsoft
  @ver 1.0
  @see Thread
  @see InCriticalSection
*/

class COMEXP CriticalSection {
public:
  CriticalSection();
  virtual ~CriticalSection();

/**
  Enters the critical section. If another thread is already in the critical
  section, the calling thread will be blocked until the other thread calls
  leave().
  @see leave()
*/
  void enter();
/**
  Leaves the critical section. If another thread is currently blocked on
  this critical section, it will be unblocked. If multiple threads are blocking
  only one will be unblocked.
  @see enter()
*/
  void leave();

/**
  Calls enter() and leave() in quick succession. Useful to make sure that no
  other thread is in the critical section (although another thread could
  immediately re-enter)
  @see enter()
  @see leave()
*/
  void inout();

private:
#ifdef ASSERTS_ENABLED
  int within;
#endif
  CRITICAL_SECTION cs;
};

/**
  This is a little helper class to ease the use of class CriticalSection.
  When it is instantiated, it enters a given critical section. When it is
  destroyed, it leaves the given critical section.
    CriticalSection a_cs;
    void blah() {
      InCriticalSection cs(a_cs); // critical section protection begins
      if (test) {
        return 0; // critical section protection ends
      }
      // critical section protection still active!
      doSomething();
      return 1; // critical section protection ends
    }
  
  @author Nullsoft
  @see CriticalSection
*/
class InCriticalSection {
public:
  InCriticalSection(CriticalSection *cs) : m_cs(cs) { m_cs->enter(); }
  InCriticalSection(CriticalSection &cs) : m_cs(&cs) { m_cs->enter(); }
  ~InCriticalSection() { m_cs->leave(); }
private:
  CriticalSection *m_cs;
};

#define INCRITICALSECTION(x) InCriticalSection __I_C_S__##__LINE__##(x)

#endif
