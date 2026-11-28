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
#ifndef _SVC_REDIR_H
#define _SVC_REDIR_H

#include "../../bfc/dispatch.h"
#include "services.h"

/* Domains:
	"Filename"
	"Url"
*/

// if you want to redirect a string easily, look at RedirString in
// bfc/util/redirstr.h

class svc_redirect : public Dispatchable {
public:
  static FOURCC getServiceType() { return WaSvc::REDIRECT; }

  int isMine(const char *str, const char *domain);
  int redirect(const char *orig_str, const char *domain, char *buf, int buflen);

protected:
  enum {
    ISMINE=100,
    REDIRECT=200,
  };
};

inline int svc_redirect::isMine(const char *str, const char *domain) {
  return _call(ISMINE, 0, str, domain);
}

inline int svc_redirect::redirect(const char *orig_str, const char *domain, char *buf, int buflen) {
  return _call(REDIRECT, 0, orig_str, domain, buf, buflen);
}

class svc_redirectI : public svc_redirect {
public:
  virtual int isMine(const char *str, const char *domain)=0;
  virtual int redirect(const char *orig_str, const char *domain, char *buf, int buflen)=0;

protected:
  RECVS_DISPATCH;
};

#include "../../bfc/svc_enum.h"
#include "../../bfc/string.h"

class RedirectEnum : public SvcEnumT<svc_redirect> {
public:
  RedirectEnum(const char *filename, const char *domain="Filename") :
    fn(filename), dom(domain) {
  }

  virtual int testService(svc_redirect *svc) {
    if (svc->isMine(fn, dom)) return 1;
    return 0;
  }

private:
  String fn, dom;
};
#endif
