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
#ifndef _SINGLEACTION_H
#define _SINGLEACTION_H

#include <bfc/std.h>

#include <studio/services/svc_action.h>
#include <studio/services/waservicefactorybase.h>

class SingleAction : public waServiceFactoryBase<svc_action, SingleAction>, public svc_actionI {
public:
  typedef void (*fnptr)(const char *param);
  static const char *getServiceName() { return "EzAction"; }

  SingleAction(const char *actionid, fnptr cbfn=NULL) : fn(cbfn) {
    registerAction(actionid, 10);
  }

  // override this or use the callback
  virtual int onSingleAction(const char *param, int p1, int p2, void *data, int datalen, RootWnd *source) {
    if (fn) { (*fn)(param); return 1; }
    return 0;
  }

private:
  virtual svc_action *newService() { return this; }
  virtual int delService(svc_action *service) { return TRUE; }

protected:
  // BU: I made this protected so you can override it if you really really
  // need to, but it's not really a SingleAction any more then, is it? ;P
  virtual int onActionId(int pvtid, const char *action, const char *param=NULL, int p1=0, int p2=0, void *data=NULL, int datalen=0, RootWnd *source=NULL) {
    if (pvtid == 10)
      return onSingleAction(param, p1, p2, data, datalen, source);
    return 0;
  }

private:
  fnptr fn;
};

#endif
