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

// Hello, and welcome to the starting point for a Wasabi app. If you're
// modifying this file for your app, just look for the EDITME markers.

#include "../wasabi/wasabi.h"	// for class WasabiKernel
#include "../wasabi/loader.h"

#ifdef USE_PROGRAM_ICON // I'm lazy and have no program icon.
#include "resource.h"
#endif

#include "../wasabi/wkc.h"
#include "../studio/ipcs.h"

// Your app should set this appropriately
#define APPNAME "Example"	//EDITME

#define WINDOWCLASSNAME "WasabiIPC_" APPNAME

// if you want to verify components and scripts before loading, modify the
// following class (and make sure USE_KERNEL_CONTROLLER is #define'd below)
#define USE_KERNEL_CONTROLLER
#ifdef USE_KERNEL_CONTROLLER
//EDITME
class MyKernelController : public WasabiKernelControllerI { 
public:
  virtual int testComponent(const char *filename) { return 1; }
  virtual int testScript(const char *filename, void *data, int datalen) { return 1; }
};
static MyKernelController mykc;
#endif

int APIENTRY WinMain(HINSTANCE hInstance,
		     HINSTANCE hPrevInstance,
		     LPSTR     lpCmdLine,
		     int       nCmdShow) {
  
#ifdef USE_TALKBACK
  // Talkback initialization
  FC_ERROR fcstatus = FC_ERROR_FAILED;
  fcstatus = FCInitialize();
	  
  // Print out error status.
  switch(fcstatus) {
	case FC_ERROR_OK:
      OutputDebugString("(QFA)Talkback managed to startup hooray crash at will commander\n");
	  break;
	case FC_ERROR_CANT_INITIALIZE:
	  OutputDebugString("(QFA)Talkback error: Can't initialize.\n");
	  break;
	case FC_ERROR_NOT_INITIALIZED:
	  OutputDebugString("(QFA)Talkback error: Not initialized.\n");
	  break;
	case FC_ERROR_ALREADY_INITIALIZED:
	  OutputDebugString("(QFA)Talkback error: Already initialized.\n");
	  break;
	case FC_ERROR_FAILED:
	  OutputDebugString("(QFA)Talkback error: Failure.\n");
	  break;
	case FC_ERROR_OUT_OF_MEMORY:
	  OutputDebugString("(QFA)Talkback error: Out of memory.\n");
	  break;
	case FC_ERROR_INVALID_PARAMETER:
	  OutputDebugString("(QFA)Talkback error: Invalid parameter.\n");
	  break;
	default:
	  OutputDebugString("(QFA)Talkback error: Unknown error status.\n");
	  break;
  }
#endif

  WasabiKernel *wasabi=NULL;

  {
    // figure out what path we're in
    char loaderpath[MAX_PATH+8];
    GetModuleFileName(hInstance, loaderpath, MAX_PATH);
    {
      char *p;
      for (p = loaderpath; *p; p++) { }	// find end
      for (; p > loaderpath && *p != '\\'; p--) { } // back up to last backslash
      *p = 0;	// kill it
    }

    // see if another instance is running
    {
      HWND h = FindWindow(WINDOWCLASSNAME, NULL);
      if (h != NULL) {	// if so, see if we should just send our command line
        char bufo[6]="";
        char fn[MAX_PATH+78];
        strcpy(fn, loaderpath);
        strcat(fn, "\\wasabi.ini");
        GetPrivateProfileString("Wasabi", "MultipleInstance", "false", bufo, sizeof(bufo), fn);
        if (strcmp(bufo, "true")) {
          // ok, send command line and exit
          COPYDATASTRUCT cd;
          cd.dwData=IpcsCommand::IPC_COMMANDLINE;
          cd.cbData=strlen(lpCmdLine)+1;
          cd.lpData=lpCmdLine;
          SendMessage(h, WM_COPYDATA, NULL, (long)&cd);
          return 0;
        }
        // otherwise we go ahead and start up
      }
    }

    // first try local dir
    wasabi = WasabiLoader::load();

    if (wasabi == NULL) {
      // if not found locally, try to find Wasabi dir from registry
      HKEY key;
      char dir[MAX_PATH];
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Nullsoft\\Wasabi\\" WASABIVERSTR, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        unsigned long type, len = sizeof(dir);
        if (RegQueryValueEx(key, "dir", NULL, &type, (unsigned char *)dir, &len) != ERROR_SUCCESS || type != REG_SZ) {
          *dir = '\0';
        }
        RegCloseKey(key);
      }

      // if not in registry...?
      if (dir[0] == '\0') {
        // eventually we'll try to fake it from $PROGRAMFILES\Wasabi\$WASABIVER
        MessageBox( NULL, "Wasabi directory not found! Try reinstalling.", NULL, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
        return 0;
      }

      // try to load the dll from the path we got from the registry
      wasabi = WasabiLoader::load(dir);
    }

    if (wasabi == NULL) { // still not loaded?!
      LPVOID lpMsgBuf;
      FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
      );
      MessageBox( NULL, (LPCTSTR)lpMsgBuf, "OS error while loading Wasabi", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
      // Free the buffer.
      LocalFree( lpMsgBuf );
      return 0;
    }

    // wasabi is loaded

    // tell wasabi what directory our app lives in
    wasabi->setAppDir(loaderpath);
  }//don't need these local vars any more

#ifdef WIN32	// only needed in win32
  wasabi->setApphInstance(hInstance);
  // here's our app icon
#ifdef USE_PROGRAM_ICON
  wasabi->setAppIcon(LoadIcon(hInstance, (LPCTSTR)IDI_STUDIO));
#endif
#endif

  // tell Wasabi our app's name
  wasabi->setAppName(APPNAME);
  // and our IPC name
  wasabi->setIPCName(WINDOWCLASSNAME);

  // if no skin is found, try to revert to default skin. usually you want this
  // behavior. if not, omit this line
  wasabi->setRevertSkinOnError(TRUE);

#ifdef USE_KERNEL_CONTROLLER
  // set up our kernel controller
  wasabi->setController(&mykc);
#endif

  // fire it up!
  int retval = wasabi->init(lpCmdLine);
  if (retval <= 0) return retval;

  // run the app
  retval = wasabi->run();

  // shut things down. don't forget to call this!
  wasabi->shutdown();

  // detach the DLL
  WasabiLoader::unload();

  return retval;
}
