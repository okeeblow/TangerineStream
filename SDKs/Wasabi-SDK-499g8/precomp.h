// precomp.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef _PRECOMP_H
#define _PRECOMP_H

#ifdef WIN32

#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#ifdef AFXMEM
#define _AFX_NOFORCE_LIBS
#define ATLASSERT(x) ASSERT(x)
#include <afx.h>
#endif

#ifdef _AFX // Are we linking with MFC ?

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#ifndef STRICT
#define STRICT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif


#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#include <afxpriv.h>        // WM_KICKIDLE
#endif // _AFX_NO_OLE_SUPPORT

#endif	// _AFX

#endif	// WIN32

// Most used wasabi headers

#include <wasabicfg.h>
#include "studio/assert.h"
#include "common/common.h"
#include "bfc/std.h"
#include "bfc/ptrlist.h"
#include "bfc/stack.h"
#include "bfc/tlist.h"
#include "bfc/string.h"
#include "bfc/dispatch.h"
#include "common/nsguid.h"
#include "studio/services/servicei.h"

#ifdef WASABI_COMPILE_WND

#include "bfc/rootwnd.h"

#ifdef WASABI_COMPILE_SKIN
#include "common/guiobjwnd.h"
#include "common/script/guiobject.h"
#endif

#endif

#ifdef WASABI_COMPILE_SCRIPT
#include "common/script/scriptobj.h"
#include "common/script/objcontroller.h"
#include "common/script/scriptvar.h"
#endif


using namespace wasabi;

#endif // _PRECOMP_H
