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
//#define NETSCAPE
// atl.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 or higher in order to build
// this project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f atlps.mak in the project directory.


// C RunTime Header Files
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define _WIN32_WINNT 0x0400
//#define _ATL_FREE_THREADED
#define _CONVERSION_USES_THREAD_LOCALE
#ifndef NETSCAPE

#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#include "atlctl.h"
#define _ATLHOST_IMPL
#include "atlhost.h"

CComModule _Module;


BEGIN_OBJECT_MAP(ObjectMap)
//	OBJECT_ENTRY(CLSID_Registrar, CRegObject)
	OBJECT_ENTRY_NON_CREATEABLE(CAxHostWindow)
END_OBJECT_MAP()
#endif

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL AtlMain(HINSTANCE hInstance)
{
#ifndef NETSCAPE
	_Module.Init(ObjectMap, hInstance, &LIBID_ATLLib);
	return AtlAxWinInit();
#else
  return FALSE;
#endif
}


extern "C"
void AtlQuit(void)
{
#ifndef NETSCAPE
  _Module.Term();
#endif
}
