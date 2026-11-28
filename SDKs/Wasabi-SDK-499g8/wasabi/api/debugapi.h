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
#ifndef __MAKIDEBUGAPI_H
#define __MAKIDEBUGAPI_H

#include "../../bfc/api/api_makidebug.h"
#include "../../studio/script/vcpudebug.h"

class MakiDebuggerApi : public api_makiDebuggerI {
  public:
    MakiDebuggerApi();
    virtual ~MakiDebuggerApi();

    virtual void debugger_trace();
    virtual int debugger_isActive();
    virtual int debugger_getVIP();
    virtual int debugger_getVSD();
    virtual int debugger_getVCC();
    virtual int debugger_getVSP();
    virtual int debugger_filterEvent(int vcpuid, int eventid);
    virtual void debugger_eventComplete();
    virtual MakiJITD *debugger_createJITD(int vcpuid, int bringitup=1);
    virtual scriptVar debugger_readStack(int n);
    virtual const char *debugger_getCodeBlock(int vcpuid);

  private:
    VCPUDebugger *debugger;
};

#endif
