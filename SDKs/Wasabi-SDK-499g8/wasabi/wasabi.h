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
#ifndef _WASABI_H
#define _WASABI_H

#include "../bfc/dispatch.h"

#define WASABIVER1_0 ((const unsigned int)((1<<16)|0))
#define WASABIVER1_1 ((const unsigned int)((1<<16)|1))
#define WASABIVER1_1STR "1.1"

#define WASABIVER WASABIVER1_1
#define WASABIVERSTR WASABIVER1_1STR

class WasabiKernelController;	 // wkc.h

class NOVTABLE WasabiKernel : public Dispatchable {
public:
/**
  Returns the Wasabi kernel version number. The loader code uses this to verify
  it has found the correct DLL to load.
  @see WasabiLoader::load()
  @ret The version number, in 16.16 fixed-point format.
*/
  int getWasabiVersion();

/**
  Sets the OSMODULEHANDLE for the DLL. This is automatically called for you by
  WasabiLoader::load().
  @see WasabiLoader::load()
*/
  void setOsModuleHandle(OSMODULEHANDLE moduleHandle);

/**
  Tells Wasabi your application directory, i.e. where your skin files and components etc. are located. Usually "c:\Program Files\AppName" or similar.
  Specify as absolute pathing.
*/
  void setAppDir(const char *path);
  void setSkinsDir(const char *path);
/**
  Sets whether Wasabi should attempt to revert to the default skin when no
  static containers are loaded at startup. Defaults to FALSE.
*/
  void setRevertSkinOnError(int rev);

/**
  Sets the OSMODULEHANDLE of the app.
*/
  void setAppOsModuleHandle(OSMODULEHANDLE moduleHandle);

/**
  Sets the app icon. Pass in the handle to a standard Windows icon that you
  wish to have show up as the app icon in the taskbar etc.
*/
  void setAppIcon(OSICONHANDLE hIcon);

/**
  Sets the application name. This will be used whereever the application name
  needs to be displayed, e.g. the main app context menu.
  @param appName Pointer to the application name.
*/
  void setAppName(const char *appName);
/**
  Sets the IPC name for the app. In win32, this is the CLASSNAME of the
  IPC window. If you don't set this it defaults to using a name based on the
  app name.
  @see setAppName
*/
  void setIPCName(const char *ipcname);

/**
  Adds a component guid to the load-only list. If this list has a guid in
  it, only components with a guid in the list will be loaded.
  @see banComponentGuid()
*/
  int addLoadOnlyComponentGuid(GUID g);
/**
  Adds a component guid to the ban list. Components with a guid in the list
  will be silently skipped during component loading. Note the ban list takes
  precedence over the load-only list.
  @see addLoadOnlyComponentGuid()
*/
  int banComponentGuid(GUID g);

/**
  Sets (optional) kernel controller. This is a callback which you can use
  to inspect each component and script before it is loaded (and veto the
  loading if need be.)
  @see WasabiKernelController
*/
  void setController(WasabiKernelController *wkc);

/**
  Initializes Wasabi. Loads components, skin files etc.
  @param cmdline The cmdline your app was started with.
  @see WasabiKernelController
  @see run()
*/
  int init(const char *cmdline);

/**
  Runs the app. After init returns successfully, call this to run the event loop
  for your app. If you need to do your own custom processing on the message
  pump you can code your own, but if not, just call this. It will return
  when the app is closed.
  @see init
  @ret The return value for the app.
*/
  int run();

/**
  Shuts down Wasabi. Be sure to call this before you terminate your app!
*/
  void shutdown();

protected:
  enum {
    GETWASABIVERSION=100,
    SETWASABIDIR=1000,
    SETSKINSDIR=1100,
    SETAPPDIR=1200,
    SETAPPOSMODULEHANDLE=2000,
    SETDLLOSMODULEHANDLE=2010,
    SETAPPICON=2020,
    SETAPPNAME=2030,
    SETIPCNAME=2040,
    SETREVERTSKIN=2050,
    SETCONTROLLER=2060,
    INIT=1023,
    RUN=9273,
    SHUTDOWN=9999,
  };
};

extern "C" {
  typedef WasabiKernel *(*GETWASABIKERNEL)();
};



// dispatchable helper functions
inline
int WasabiKernel::getWasabiVersion() {
  return _call(GETWASABIVERSION, WASABIVER1_0);
}

inline
void WasabiKernel::setAppDir(const char *path) {
  _voidcall(SETAPPDIR, path);
}

inline
void WasabiKernel::setSkinsDir(const char *path) {
  _voidcall(SETSKINSDIR, path);
}

inline
void WasabiKernel::setRevertSkinOnError(int rev) {
  _voidcall(SETREVERTSKIN, rev);
}

inline
void WasabiKernel::setOsModuleHandle(OSMODULEHANDLE modulehandle) {
  _voidcall(SETDLLOSMODULEHANDLE, modulehandle);
}

inline
void WasabiKernel::setAppOsModuleHandle(OSMODULEHANDLE modulehandle) {
  _voidcall(SETAPPOSMODULEHANDLE, modulehandle);
}

inline
void WasabiKernel::setAppIcon(HICON hIcon) {
  _voidcall(SETAPPICON, hIcon);
}

inline
void WasabiKernel::setAppName(const char *appName) {
  _voidcall(SETAPPNAME, appName);
}

inline
void WasabiKernel::setIPCName(const char *ipcname) {
  _voidcall(SETIPCNAME, ipcname);
}

inline
void WasabiKernel::setController(WasabiKernelController *wkc) {
  _voidcall(SETCONTROLLER, wkc);
}

inline
int WasabiKernel::init(const char *cmdline) {
  return _call(INIT, 0, cmdline);
}

inline
int WasabiKernel::run() {
  return _call(RUN, 0);
}

inline
void WasabiKernel::shutdown() {
  _voidcall(SHUTDOWN);
}

#endif
