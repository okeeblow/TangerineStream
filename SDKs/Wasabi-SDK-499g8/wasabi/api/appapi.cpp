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
#include <wasabicfg.h>
#include "appapi.h"
#include "../../studio/pathmgr.h"
#include "../../studio/version.h"

#ifdef WA3COMPATIBILITY
#include "../../studio/main.h" // CUT!
#include "../../studio/layout.h" // CUT!
#include "../../studio/skinparse.h"
#endif

String g_resourcepath;

// {3CBD4483-DC44-11d3-B608-000086340885}
static const GUID _baseGUID = 
{ 0x3cbd4483, 0xdc44, 0x11d3, { 0xb6, 0x8, 0x0, 0x0, 0x86, 0x34, 0x8, 0x85 } };

api_application *applicationApi = NULL;

ApplicationApi::ApplicationApi(HINSTANCE instance) : appInstance(instance), shuttingdown(0) {
#ifndef _WASABIRUNTIME

#ifdef WASABI_DIRS_FROMEXEPATH
  HINSTANCE hInst = GetModuleHandle(0);
#else
  HINSTANCE hInst = appInstance;
#endif

  String path;
  {
    char g[WA_MAX_PATH];
    GetModuleFileName(hInst,g,WA_MAX_PATH-1);
    char *p = STRRCHR(g, '\\');
    if (p) *p = 0;
    path = g;
  }

  g_resourcepath = path;
  apppath = path;

  g_resourcepath.cat("/");
#if defined(WASABI_COMPILE_SKIN) || defined(WASABI_COMPILE_IMGLDR)
  g_resourcepath.cat(WASABI_RESOURCES_SUBDIRECTORY);
#endif

#endif

#ifdef WIN32
  HANDLE h = NULL;
  DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &h, 0, FALSE, DUPLICATE_SAME_ACCESS);
  mainthread = h;	// 32-bit writes assumed atomic
#endif
#ifdef LINUX
  DebugString("portme: api main thread handle dup\n");
#endif
}

ApplicationApi::~ApplicationApi() {
  g_resourcepath.purge();
}

const char *ApplicationApi::main_getAppName() {
  return WasabiVersion::getAppName();
}

const char *ApplicationApi::main_getVersionString() {
  return WasabiVersion::getVersionString();
}

unsigned int ApplicationApi::main_getBuildNumber() {
  return WasabiVersion::getBuildNumber();
}

GUID ApplicationApi::main_getGUID() {
  return guid;
}

HANDLE ApplicationApi::main_getMainThreadHandle() {
  if (mainthread == NULL) return NULL;
  HANDLE h = NULL;
#ifdef WIN32
  DuplicateHandle(GetCurrentProcess(), mainthread, GetCurrentProcess(), &h, 0, FALSE, DUPLICATE_SAME_ACCESS);
#endif
#ifdef LINUX
  DebugString("portme: dup handle\n");
#endif
  return h;
}

HINSTANCE ApplicationApi::main_gethInstance() {
  return appInstance;
}

const char *ApplicationApi::main_getCommandLine() {
  return cmdLine;
}

void ApplicationApi::main_shutdown(int deferred) {
  shuttingdown = 1;
#ifdef _WASABIRUNTIME
  Main::doAction(ACTION_CLOSE, deferred);
#endif
#ifdef WASABI_CUSTOM_QUIT
WASABI_CUSTOM_QUITFN
#endif

}

void ApplicationApi::main_cancelShutdown() {
#ifdef _WASABIRUNTIME
  Main::cancelShutdown();
#endif
  shuttingdown = 0;
}

const char *ApplicationApi::path_getUserSettingsPath() {
  return PathMgr::getUserSettingsPath();
}

const char *ApplicationApi::path_getAppPath() {
#ifdef _WASABIRUNTIME
  return Main::getMainAppPath();
#endif
  return apppath;
}

int ApplicationApi::main_isShuttingDown() {
  return shuttingdown;
}

void ApplicationApi::setHInstance(HINSTANCE instance) {
  appInstance = instance;
};

void ApplicationApi::setCommandLine(const char *cmdline) {
  cmdLine = cmdline;
}

void ApplicationApi::setGUID(GUID g) {
  guid = g;
}

