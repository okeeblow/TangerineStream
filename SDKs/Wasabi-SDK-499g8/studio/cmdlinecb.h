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
#ifndef _CMDLINECB_H
#define _CMDLINECB_H

#include "syscbi.h"

namespace  CmdLineCallback {
enum {
  PROCESS=1,
};
};

#define CMDLINECALLBACK_PARENT SysCallbackI
class CmdLineCallbackI : public CMDLINECALLBACK_PARENT {
protected:
  CmdLineCallbackI() {}

public:
  virtual int cmdlinecb_onProcess(const char *cmdline, int seqno)=0;

private:
  virtual FOURCC syscb_getEventType() { return SysCallback::CMDLINE; }
  virtual int syscb_notify(int msg, int param1=0, int param2=0) {
    switch (msg) {
      case CmdLineCallback::PROCESS:
        return cmdlinecb_onProcess(reinterpret_cast<const char *>(param1), param2);
    }
    return 0;
  }
};

#endif
