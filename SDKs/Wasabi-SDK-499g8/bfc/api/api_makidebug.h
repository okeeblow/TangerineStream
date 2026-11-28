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
#ifndef __API_MAKIDEBUG_H
#define __API_MAKIDEBUG_H

/*---------------------------------------------------------
api_makiDebugger
  void debugger_trace();
  int debugger_isActive();
  int debugger_getVIP();
  int debugger_getVSD();
  int debugger_getVCC();
  int debugger_getVSP();
  int debugger_filterEvent(int vcpuid, int eventid);
  void debugger_eventComplete();
  MakiJITD *debugger_createJITD(int vcpuid, int bringitup=1);
  int debugger_readStack(int n);
  int debugger_getCodeBlock(int vcpuid);
---------------------------------------------------------*/

#include <wasabicfg.h>

#ifndef WASABI_COMPILE_SCRIPT
#error "This module requires the script api" 
#endif 

#ifndef WASABI_COMPILE_MAKIDEBUG
#error "This module requires the script debugger api" 
#endif 

#include "../dispatch.h"
#include "common/script/scriptvar.h"

class MakiJITD;

class api_makiDebugger : public Dispatchable {
  public:
    void debugger_trace();
    int debugger_isActive();
    int debugger_getVIP();
    int debugger_getVSD();
    int debugger_getVCC();
    int debugger_getVSP();
    int debugger_filterEvent(int vcpuid, int eventid);
    void debugger_eventComplete();
    MakiJITD *debugger_createJITD(int vcpuid, int bringitup=1);
    scriptVar debugger_readStack(int n);
    const char *debugger_getCodeBlock(int vcpuid);

  enum {
    API_MAKIDEBUGGER_DEBUGGER_TRACE = 0,
    API_MAKIDEBUGGER_DEBUGGER_ISACTIVE = 10,
    API_MAKIDEBUGGER_DEBUGGER_GETVIP = 20,
    API_MAKIDEBUGGER_DEBUGGER_GETVSD = 30,
    API_MAKIDEBUGGER_DEBUGGER_GETVCC = 40,
    API_MAKIDEBUGGER_DEBUGGER_GETVSP = 50,
    API_MAKIDEBUGGER_DEBUGGER_FILTEREVENT = 60,
    API_MAKIDEBUGGER_DEBUGGER_EVENTCOMPLETE = 70,
    API_MAKIDEBUGGER_DEBUGGER_CREATEJITD = 80,
    API_MAKIDEBUGGER_DEBUGGER_READSTACK = 90,
    API_MAKIDEBUGGER_DEBUGGER_GETCODEBLOCK = 100,
  };
};

inline void api_makiDebugger::debugger_trace() {
  _voidcall(API_MAKIDEBUGGER_DEBUGGER_TRACE);
}

inline int api_makiDebugger::debugger_isActive() {
  return _call(API_MAKIDEBUGGER_DEBUGGER_ISACTIVE, (int)0);
}

inline int api_makiDebugger::debugger_getVIP() {
  return _call(API_MAKIDEBUGGER_DEBUGGER_GETVIP, (int)0);
}

inline int api_makiDebugger::debugger_getVSD() {
  return _call(API_MAKIDEBUGGER_DEBUGGER_GETVSD, (int)0);
}

inline int api_makiDebugger::debugger_getVCC() {
  return _call(API_MAKIDEBUGGER_DEBUGGER_GETVCC, (int)0);
}

inline int api_makiDebugger::debugger_getVSP() {
  return _call(API_MAKIDEBUGGER_DEBUGGER_GETVSP, (int)0);
}

inline int api_makiDebugger::debugger_filterEvent(int vcpuid, int eventid) {
  return _call(API_MAKIDEBUGGER_DEBUGGER_FILTEREVENT, (int)0, vcpuid, eventid);
}

inline void api_makiDebugger::debugger_eventComplete() {
  _voidcall(API_MAKIDEBUGGER_DEBUGGER_EVENTCOMPLETE);
}

inline MakiJITD *api_makiDebugger::debugger_createJITD(int vcpuid, int bringitup) {
  return _call(API_MAKIDEBUGGER_DEBUGGER_CREATEJITD, (MakiJITD *)NULL, vcpuid, bringitup);
}

inline const char *api_makiDebugger::debugger_getCodeBlock(int vcpuid) {
  return _call(API_MAKIDEBUGGER_DEBUGGER_GETCODEBLOCK, (const char *)NULL, vcpuid);
}

inline scriptVar api_makiDebugger::debugger_readStack(int n) {
  scriptVar v={0,0};
  return _call(API_MAKIDEBUGGER_DEBUGGER_READSTACK, v, n);
}

class api_makiDebuggerI : public api_makiDebugger {
  public:
    virtual void debugger_trace()=0;
    virtual int debugger_isActive()=0;
    virtual int debugger_getVIP()=0;
    virtual int debugger_getVSD()=0;
    virtual int debugger_getVCC()=0;
    virtual int debugger_getVSP()=0;
    virtual int debugger_filterEvent(int vcpuid, int eventid)=0;
    virtual void debugger_eventComplete()=0;
    virtual MakiJITD *debugger_createJITD(int vcpuid, int bringitup=1)=0;
    virtual scriptVar debugger_readStack(int n)=0;
    virtual const char *debugger_getCodeBlock(int vcpuid)=0;

  protected:
    RECVS_DISPATCH;
};

// {858E4B64-AF1E-4b64-8D27-EFFAD9F82BB4}
static const GUID makiDebugApiServiceGuid = 
{ 0x858e4b64, 0xaf1e, 0x4b64, { 0x8d, 0x27, 0xef, 0xfa, 0xd9, 0xf8, 0x2b, 0xb4 } };

extern api_makiDebugger *debugApi;

#endif


