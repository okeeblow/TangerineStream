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
#ifndef _WKC_H
#define _WKC_H

#include "../bfc/dispatch.h"

/**
  A callback class for the loader so it can determine which resources wasabi.dll should load.
*/
class NOVTABLE WasabiKernelController : public Dispatchable {
public:
/**
  Check if a component should load.
*/
  int testComponent(const char *filename);
  int testScript(const char *filename, void *data, int datalen);
  int testSkin(const char *skinname);
  int testSkinFile(const char *filename);

protected:
  enum {
    TESTCOMPONENT=1000,
    TESTSCRIPT=2000,
    TESTSKIN=3000,
    TESTSKINFILE=4000,
  };
};

inline
int WasabiKernelController::testComponent(const char *filename) {
  return _call(TESTCOMPONENT, TRUE, filename);
}

inline
int WasabiKernelController::testScript(const char *filename, void *data, int datalen) {
  return _call(TESTSCRIPT, TRUE, filename, data, datalen);
}

inline
int WasabiKernelController::testSkin(const char *skinname) {
  return _call(TESTSKIN, TRUE, skinname);
}

inline
int WasabiKernelController::testSkinFile(const char *filename) {
  return _call(TESTSKINFILE, TRUE, filename);
}

// implementors derive from this one
class WasabiKernelControllerI : public WasabiKernelController {
public:
  // default to OK
  virtual int testComponent(const char *filename) { return 1; }
  virtual int testScript(const char *filename, void *data, int datalen) { return 1; }
  virtual int testSkin(const char *skinname) { return 1; }
  virtual int testSkinFile(const char *filename) { return 1; }

protected:
  RECVS_DISPATCH;
};

#endif
