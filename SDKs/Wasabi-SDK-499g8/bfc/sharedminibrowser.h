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
#ifndef _SHAREDMINIBROWSER_H
#define _SHAREDMINIBROWSER_H

#include "../studio/skincb.h"

class SkinMonitor;

class SharedMiniBrowser {
public:
  static void navigateUrl(const char *url);
  static void shutdown();

  static int m_inserted;
  static SkinMonitor *m_monitor;

};

class SkinMonitor : public SkinCallbackI {
public:
  SkinMonitor() {
    WASABI_API_SYSCB->syscb_registerCallback(this);
  }
  virtual ~SkinMonitor() {
    WASABI_API_SYSCB->syscb_deregisterCallback(this);
  }
  virtual int skincb_onReset() { 
    SharedMiniBrowser::m_inserted = 0;
    return 0;
  }
};


#endif // _SHAREDMINIBROWSER_H
