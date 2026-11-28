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
#ifndef __APPAPI_H
#define __APPAPI_H

#include "../../bfc/api/api_app.h"
#include "../../bfc/string.h"

extern String g_resourcepath;

class ApplicationApi : public api_applicationI {
  public:
     ApplicationApi(HINSTANCE hinstance);
     virtual ~ApplicationApi();

     const char *main_getAppName();
     const char *main_getVersionString();
     unsigned int main_getBuildNumber();
     GUID main_getGUID();
     HANDLE main_getMainThreadHandle();
     HINSTANCE main_gethInstance();
     const char *main_getCommandLine();
     void main_shutdown(int deferred = TRUE);
     void main_cancelShutdown();
     const char *path_getAppPath();
     const char *path_getUserSettingsPath();
     int main_isShuttingDown();

     void setHInstance(HINSTANCE instance);
     void setCommandLine(const char *str);
     void setGUID(GUID g);

  protected:
    
    HINSTANCE appInstance;
    String cmdLine;
    HANDLE mainthread;
    GUID guid;
    int shuttingdown;
    String apppath;
};

extern ApplicationApi _applicationApi;

#endif
