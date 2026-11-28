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

#include "loader.h"
#include "wasabi.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define WASABIDLL "wasabi.dll"
#endif
#ifdef LINUX
#define WASABIDLL "wasabi.so"
#endif
//PORTME


static OSMODULEHANDLE library;

WasabiKernel *WasabiLoader::load(const char *directory) {
  if (directory == NULL) directory = "";
  char fn[MAX_PATH*2];
  strcpy(fn, directory);
  if (*fn != '\0') strcat(fn, "/");	// might be relative
  strcat(fn, WASABIDLL);

  if (library == NULL) {
#ifdef WIN32
    library = LoadLibrary(fn);
#endif
#ifdef LINUX
     library = dlopen( fn, RTLD_LAZY );
     if (library == NULL) fprintf( stderr, "%s\n", dlerror() );
#endif
//PORTME
    if (library == NULL) return NULL;
  }

  GETWASABIKERNEL gwk = NULL;
#ifdef WIN32
  gwk = (GETWASABIKERNEL)GetProcAddress(library, "wasabi_getKernel");
#endif
#ifdef LINUX
  gwk = (GETWASABIKERNEL)dlsym(library,"wasabi_getKernel");
#endif
//PORTME
  if (gwk == NULL) {
    unload();
    return NULL;
  }

  // get the kernel pointer from the function
  WasabiKernel *wk = (*gwk)();

  // version check
  if (wk->getWasabiVersion() != WASABIVER) {
    unload();
    return NULL;
  }

  // inform wasabi.dll as to its OSMODULEHANDLE
  wk->setOsModuleHandle(library);

  return wk;
}

void WasabiLoader::unload() {
  if (library != NULL) {
#ifdef WIN32
    FreeLibrary(library);
#endif
#ifdef LINUX
    dlclose(library);
#endif
//PORTME
  }
  library = NULL;
}
