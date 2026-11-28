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
#include "api_makidebug.h"

#ifdef CBCLASS
#undef CBCLASS
#endif
#define CBCLASS api_makiDebuggerI
START_DISPATCH;
  VCB(API_MAKIDEBUGGER_DEBUGGER_TRACE, debugger_trace);
  CB(API_MAKIDEBUGGER_DEBUGGER_ISACTIVE, debugger_isActive);
  CB(API_MAKIDEBUGGER_DEBUGGER_GETVIP, debugger_getVIP);
  CB(API_MAKIDEBUGGER_DEBUGGER_GETVSD, debugger_getVSD);
  CB(API_MAKIDEBUGGER_DEBUGGER_GETVCC, debugger_getVCC);
  CB(API_MAKIDEBUGGER_DEBUGGER_GETVSP, debugger_getVSP);
  CB(API_MAKIDEBUGGER_DEBUGGER_FILTEREVENT, debugger_filterEvent);
  VCB(API_MAKIDEBUGGER_DEBUGGER_EVENTCOMPLETE, debugger_eventComplete);
  CB(API_MAKIDEBUGGER_DEBUGGER_CREATEJITD, debugger_createJITD);
  CB(API_MAKIDEBUGGER_DEBUGGER_READSTACK, debugger_readStack);
  CB(API_MAKIDEBUGGER_DEBUGGER_GETCODEBLOCK, debugger_getCodeBlock);
END_DISPATCH;
