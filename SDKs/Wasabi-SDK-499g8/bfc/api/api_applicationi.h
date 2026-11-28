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
#ifndef __API_APPLICATIONI_IMPL_H
#define __API_APPLICATIONI_IMPL_H

/*<?<autoheader/>*/
#include "api_application.h"
#include "api_applicationx.h"
/*?>*/

/*[interface.header.h]
#include "common/nsGUID.h"
class String;
*/

class api_applicationI : public api_applicationX {
public:
  NODISPATCH api_applicationI(HINSTANCE instance);
  NODISPATCH virtual ~api_applicationI();

  DISPATCH(10) const char *main_getAppName();
  DISPATCH(20) const char *main_getVersionString();
  DISPATCH(30) unsigned int main_getBuildNumber();
  DISPATCH(40) GUID main_getGUID();
  DISPATCH(50) HANDLE main_getMainThreadHandle();
  DISPATCH(60) HINSTANCE main_gethInstance();
  DISPATCH(70) const char *main_getCommandLine();
  DISPATCH(80) void main_shutdown(int deferred = TRUE);
  DISPATCH(90) void main_cancelShutdown();
  DISPATCH(100) int main_isShuttingDown();
  DISPATCH(110) const char *path_getAppPath();
  DISPATCH(120) const char *path_getUserSettingsPath();
  DISPATCH(130) int app_getInitCount();

  NODISPATCH void setHInstance(HINSTANCE instance);
  NODISPATCH void setCommandLine(const char *cmdline);
  NODISPATCH void setGUID(GUID g);

protected:
    
  HINSTANCE appInstance;
  String cmdLine;
  HANDLE mainthread;
  GUID guid;
  int shuttingdown;
  String apppath;
};

/*[interface.footer.h]
// {23B96771-09D7-46d3-9AE2-20DCEA6C86EA}
static const GUID applicationApiServiceGuid = 
{ 0x23b96771, 0x9d7, 0x46d3, { 0x9a, 0xe2, 0x20, 0xdc, 0xea, 0x6c, 0x86, 0xea } };

extern api_application *applicationApi;
extern String g_resourcepath;
*/


#endif // __API_APPLICATIONI_IMPL_H
