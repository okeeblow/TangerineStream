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
#include "debugapi.h"
#include "../../studio/script/jitd.h"
#include "../../studio/script/vcpu.h"

api_makiDebugger *debugApi = NULL;

MakiDebuggerApi::MakiDebuggerApi() {
  debugger = new VCPUDebugger();
}

MakiDebuggerApi::~MakiDebuggerApi() {
  delete debugger;
}

void MakiDebuggerApi::debugger_trace() {
  debugger->trace();
}

int MakiDebuggerApi::debugger_isActive() {
  return debugger->isActive();
}

int MakiDebuggerApi::debugger_getVIP() {
  return VCPU::VIP;
}

int MakiDebuggerApi::debugger_getVSD() {
  return VCPU::VSD;
}

int MakiDebuggerApi::debugger_getVCC() {
  return VCPU::VCC;
}

int MakiDebuggerApi::debugger_getVSP() {
  return VCPU::VSP;
}

int MakiDebuggerApi::debugger_filterEvent(int vcpuid, int eventid) {
  return debugger->filterEvent(vcpuid, eventid);
}

void MakiDebuggerApi::debugger_eventComplete() {
  debugger->eventComplete();
}

MakiJITD *MakiDebuggerApi::debugger_createJITD(int vcpuid, int bringitup) {
  MakiJITD *jitd = debugger->createJITD(vcpuid);
  if (bringitup)
    jitd->setGlobalBreakpoint(1);
  return jitd;
}

scriptVar MakiDebuggerApi::debugger_readStack(int n) {
  VCPUscriptVar v = VCPU::peekAt(n);
  return v.v;
}

const char *MakiDebuggerApi::debugger_getCodeBlock(int vcpuid) {
  return VCPU::getCodeBlock(vcpuid);
}
