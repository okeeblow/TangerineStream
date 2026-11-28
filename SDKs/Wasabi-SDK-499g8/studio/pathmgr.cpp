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
#include "pathmgr.h"
#include "api.h"
#include "../bfc/util/inifile.h"

#if !defined(WIN32) && !defined(LINUX)
#error port me
#endif

#ifdef WIN32
#include <shlobj.h>
#endif

#define INIFILE "wasabi.ini"

static int checkMU() {
  String appp = WASABI_API_APP->path_getAppPath();
  Std::fileInfoStruct fis;
  Std::getFileInfos(appp, &fis);
  if (fis.readonly) return TRUE;
#if 0//eh, let's just trust that the dir would be readonly
#ifdef WIN32
  String apdir = appp;
  apdir.trunc(3);	// "c:\"
  if (GetDriveType(apdir) == DRIVE_CDROM) return TRUE;
#endif
#endif
  return IniFile(StringPrintf("%s%s%s", WASABI_API_APP->path_getAppPath(), DIRCHARSTR, INIFILE)).getBool("Wasabi", "MultiUser", FALSE);
}

static void makeUserDir(String &dir) {
#ifdef WIN32
  ITEMIDLIST *lpiil;
  int r = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA,  &lpiil);
  if (r != S_OK) {
    ASSERTALWAYS("no home dir found");
  }
  char buf[MAX_PATH];
  r = SHGetPathFromIDList(lpiil, buf);
  if (r == FALSE) {
    ASSERTALWAYS("path fuct");
  }

  LPMALLOC ppm;
  if (CoGetMalloc(1, &ppm) == S_OK) {
    ppm->Free(lpiil);
    ppm->Release();
  }

  dir = buf;
  dir.cat(DIRCHARSTR);
  dir.cat(WASABI_API_APP->main_getAppName());
  Std::createDirectory(dir);
#endif
#ifdef LINUX
  dir = "~/";	// hee hee
#endif
//PORTME
  ASSERT(!dir.isempty());
}

const char *PathMgr::getUserSettingsPath() {
  static String ret;
  if (ret.isempty()) {
    if (!checkMU()) ret = WASABI_API_APP->path_getAppPath();
    else {
      // create per-user dir string
      makeUserDir(ret);
    }
  }
ASSERT(!Std::isDirChar(ret.lastChar()));
  return ret;
}

#ifdef WASABI_COMPILE_COMPONENTS

const char *PathMgr::getComponentDataPath(GUID g) {
  // for now, we ignore GUID
  static String ret;
  if (ret.isempty()) {
    if (!checkMU()) {
      ret = WASABI_API_APP->path_getAppPath();
      ret.cat(DIRCHARSTR);
      ret.cat("Wacs");
      ret.cat(DIRCHARSTR);
      ret.cat("data");
    } else {
      // create per-user dir string
      makeUserDir(ret);
      ret.cat(DIRCHARSTR);
      ret.cat("data");
      Std::createDirectory(ret);
    }
  }
  return ret;
}

#endif
