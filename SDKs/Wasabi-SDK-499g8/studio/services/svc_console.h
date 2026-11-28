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
#ifndef _SVC_CONSOLE_H
#define _SVC_CONSOLE_H

#include "../../common/dispatch.h"

class svc_console : public Dispatchable {
public:
  int activated();
  int outputString(int severity, const char *string);

  enum {
    ACTIVATED=10,
    OUTPUTSTRING=20,
  };
};

inline int svc_console::activated() {
  return _call(ACTIVATED, 0);
}

inline int svc_console::outputString(int severity, const char *string) {
  return _call(OUTPUTSTRING, 0, severity, string);
}

// derive from this one
class svc_consoleI : public svc_console {
public:
  virtual int activated()=0;
  virtual int outputString(int severity, const char *string)=0;

protected:
  RECVS_DISPATCH;
};

#endif
