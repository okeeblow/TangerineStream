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
#ifndef _EZFEED_H
#define _EZFEED_H

#include "../std.h"
#include "../../studio/services/waservicefactorybase.h"
#include "../textfeed.h"

class COMEXP EzFeed : public waServiceFactoryBase<svc_textFeed, EzFeed>, public TextFeed {
public:
  EzFeed() : registered(0) { }
  static const char *getServiceName() { return "EzTextFeed"; }
  virtual svc_textFeed *newService() { return this; }
  virtual int delService(svc_textFeed *service) { return TRUE; }

  virtual int svc_notify(int msg, int param1 = 0, int param2 = 0) {
    if (msg == SvcNotify::ONREGISTERED) registered = 1;
    else if (msg == SvcNotify::ONDEREGISTERED) registered = 0;
    return 1;
  }

private:
  int registered;
};

#endif
