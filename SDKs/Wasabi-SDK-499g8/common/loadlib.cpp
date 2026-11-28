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

#include "loadlib.h"

#if !defined(WIN32) && !defined(LINUX)
#error port me
#endif

Library::Library(const char *filename) : Named(filename) {
  lib = NULL;
}

Library::~Library() {
  unload();
}

int Library::load(const char *newname) {
  if (lib != NULL && newname == NULL) return 1;
  unload();
  if (newname != NULL) setName(newname);

  const char *n = getName();
  ASSERT(n != NULL);
#ifdef WIN32
  __try {
    lib = LoadLibrary(n);
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    // stupid DLL
    lib = NULL;
OutputDebugString("exception while loading dll");
OutputDebugString(newname);
OutputDebugString("\n");
  }
#elif defined(LINUX)
  // Not using string to try to not use common/wasabi in Studio.exe
  char *conv = strdup( getName() );
  int len = strlen( conv );
  if ( ! strcasecmp( conv + len - 4, ".dll" ) ) {
    strcpy( conv + len - 4, ".so" );
  }

  lib = dlopen(conv, RTLD_LAZY);

  free( conv );
#else
#error port me!
#endif
  if (lib == NULL) return 0;
  return 1;
}

void Library::unload() {
  if (lib != NULL) {
#ifdef WIN32
    FreeLibrary(lib);
#elif defined(LINUX)
    dlclose(lib);
#else
#error port me!
#endif
  }
  lib = NULL;
}

void *Library::getProcAddress(const char *procname) {
  ASSERT(procname != NULL);
#if defined(WIN32)
  return GetProcAddress(lib, procname);
#elif defined(LINUX)
  return dlsym(lib,procname);
#else
#error port me!
#endif
}
