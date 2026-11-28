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
#ifndef _DEBUG
// release optimizations
// /Og (global optimizations), /Os (favor small code), /Oy (no frame pointers)
//#pragma optimize("gsy",on)
#pragma comment(linker,"/RELEASE")
// set the 512-byte alignment (only in VC6+)
#if _MSC_VER >= 1200
#pragma comment(linker,"/opt:nowin98")
#endif
#endif

#define STRICT
#include <windows.h>
#include "out.h"

extern Out_Module mod;


#define NAME "DirectSound output v1.4 beta 6"

extern UINT cfg_def_fade;

typedef struct
{
	UINT time;
	const char * name;
	bool on,usedef;
	UINT get_time() {return on ? (usedef ? cfg_def_fade : time) : 0;}
} FADE_CFG;

extern FADE_CFG cfg_fade_start,cfg_fade_firststart,cfg_fade_stop,cfg_fade_pause,cfg_fade_seek;
extern bool cfg_oldpause;
extern bool cfg_createprimary;