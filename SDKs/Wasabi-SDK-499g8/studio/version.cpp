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
#include "precomp.h"
#include "version.h"
#include "bfc/util/pathparse.h"

static int buildno = -1;
static const char *WASABI_VERSION = "";

#define STRFILEVER     "1, 0, 0, 499\0"

static String appname, version_string;

void WasabiVersion::setAppName(const char *name) {
  appname = name;
}

const char *WasabiVersion::getAppName() {
  return appname;
}

const char *WasabiVersion::getVersionString() {
  if (appname.isempty()) appname="";
  if (version_string.isempty()) {
    if (!appname.isempty())
      version_string = appname.getValue();
    if (STRLEN(WASABI_VERSION)) {
      version_string.cat(" ");
      version_string.cat(WASABI_VERSION);
    }
  }
  return version_string;
}

unsigned int WasabiVersion::getBuildNumber() {
  if (buildno == -1) {
    PathParser pp(STRFILEVER, ",");
    buildno = ATOI(pp.enumString(3));
  }
  return buildno;
}
