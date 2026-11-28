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
#ifndef __API_INIT_H
#define __API_INIT_H

#include "studio/api.h"
#include "bfc/platform/platform.h"
#ifdef WASABI_COMPILE_SKIN
#include "studio/widgets.h"
#include "bfc/string.h"
#endif

#include "bfc/api/api_applicationi.h"
#include "bfc/api/api_servicei.h"

#ifdef WASABI_COMPILE_SYSCB
#include "bfc/api/api_syscbi.h"
#endif

#include <studio/services/service.h>

#ifdef WASABI_COMPILE_CONFIG
class api_configI;
#endif
#ifdef WASABI_COMPILE_SKIN
class SkinApi;
#endif
#ifdef WASABI_COMPILE_SCRIPT
class api_makiI;
#endif
#ifdef WASABI_COMPILE_MEMMGR
class MemMgrApi;
#endif
#ifdef WASABI_COMPILE_TIMERS
class TimerApi;
#endif
#ifdef WASABI_COMPILE_WND
class WndApi;
class ForeignWnd;
#endif
#ifdef WASABI_COMPILE_XMLPARSER
class XmlApi;
#endif
#ifdef WASABI_COMPILE_IMGLDR
class ImgLdrApi;
#endif
#ifdef WASABI_COMPILE_FONTS
class FontApi;
#endif
#ifdef WASABI_COMPILE_WNDMGR
class WndMgrApi;
#endif
#ifdef WASABI_COMPILE_MAKIDEBUG
class MakiDebuggerApi;
#endif
#ifdef WASABI_COMPILE_FILEREADER
class FileReaderApi;
#endif
#ifdef WASABI_COMPILE_LOCALES
class api_localesI;
#endif
#ifndef WA3COMPATIBILITY
#ifdef WASABI_COMPILE_MEDIACORE
class Core;
#endif
#endif

class ApplicationApiService : public waServiceBase<api_application, ApplicationApiService> {
public:
  ApplicationApiService() : waServiceBase<api_application, ApplicationApiService>(applicationApiServiceGuid) {}
  static const char *getServiceName() { return "Application API"; }
  virtual api_application *getService() { return applicationApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#ifdef WASABI_COMPILE_SYSCB

#include "bfc/api/api_syscb.h"

class SysCBApiService : public waServiceBase<api_syscb, SysCBApiService> {
public:
  SysCBApiService() : waServiceBase<api_syscb, SysCBApiService>(syscbApiServiceGuid) {}
  static const char *getServiceName() { return "System Callback API"; }
  virtual api_syscb *getService() { return sysCallbackApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_SYSCB


#ifdef WASABI_COMPILE_MEDIACORE

#include "bfc/api/api_core.h"

class CoreApiService : public waServiceBase<api_core, CoreApiService> {
public:
  CoreApiService() : waServiceBase<api_core, CoreApiService>(coreApiServiceGuid) {}
  static const char *getServiceName() { return "Core API"; }
  virtual api_core *getService() { return coreApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_MEDIACORE

#ifdef WASABI_COMPILE_FILEREADER

#include "bfc/api/api_filereader.h"

class FileReaderApiService : public waServiceBase<api_fileReader, FileReaderApiService> {
public:
  FileReaderApiService() : waServiceBase<api_fileReader, FileReaderApiService>(fileReaderApiServiceGuid) {}
  static const char *getServiceName() { return "FileReader API"; }
  virtual api_fileReader *getService() { return fileApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_FILEREADER

#ifdef WASABI_COMPILE_CONFIG

#include "bfc/api/api_config.h"

class ConfigApiService : public waServiceBase<api_config, ConfigApiService> {
public:
  ConfigApiService() : waServiceBase<api_config, ConfigApiService>(configApiServiceGuid) {}
  static const char *getServiceName() { return "Config API"; }
  virtual api_config *getService() { return configApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_CONFIG

#ifdef WASABI_COMPILE_IMGLDR

#include "bfc/api/api_imgldr.h"

class ImgLdrApiService: public waServiceBase<imgldr_api, ImgLdrApiService> {
public:
  ImgLdrApiService() : waServiceBase<imgldr_api, ImgLdrApiService>(imgLdrApiServiceGuid) {}
  static const char *getServiceName() { return "Image Loading API"; }
  virtual imgldr_api *getService() { return imgLoaderApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_IMGLDR

#ifdef WASABI_COMPILE_FONTS

#include "bfc/api/api_font.h"

class FontApiService : public waServiceBase<api_font, FontApiService> {
public:
  FontApiService() : waServiceBase<api_font, FontApiService>(fontApiServiceGuid) {}
  static const char *getServiceName() { return "Font Rendering API"; }
  virtual api_font *getService() { return fontApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_FONT

#ifdef WASABI_COMPILE_XMLPARSER

#include "bfc/api/api_xml.h"

class XmlApiService : public waServiceBase<api_xml, XmlApiService> {
public:
  XmlApiService() : waServiceBase<api_xml, XmlApiService>(xmlApiServiceGuid) {}
  static const char *getServiceName() { return "XML Parser API"; }
  virtual api_xml *getService() { return xmlApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_XMLPARSER

#ifdef WASABI_COMPILE_LOCALES

#include "bfc/api/api_locales.h"

class LocalesApiService : public waServiceBase<api_locales, LocalesApiService> {
public:
  LocalesApiService() : waServiceBase<api_locales, LocalesApiService>(localesApiServiceGuid) {}
  static const char *getServiceName() { return "Locales API"; }
  virtual api_locales *getService() { return localesApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_LOCALES

#ifdef WASABI_COMPILE_TIMERS

#include "bfc/api/api_timer.h"

class TimerApiService : public waServiceBase<timer_api, TimerApiService> {
public:
  TimerApiService() : waServiceBase<timer_api, TimerApiService>(timerApiServiceGuid) {}
  static const char *getServiceName() { return "Timer API"; }
  virtual timer_api *getService() { return timerApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_TIMERS

#ifdef WASABI_COMPILE_MEMMGR

#include "bfc/api/api_memmgr.h"

class MemMgrApiService : public waServiceBase<api_memmgr, MemMgrApiService> {
public:
  MemMgrApiService() : waServiceBase<api_memmgr, MemMgrApiService>(memMgrApiServiceGuid) {}
  static const char *getServiceName() { return "Memory Manager API"; }
  virtual api_memmgr *getService() { return memmgrApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_TIMERS

#ifdef WASABI_COMPILE_WND

#include "bfc/api/api_wnd.h"

class WndApiService : public waServiceBase<wnd_api, WndApiService> {
public:
  WndApiService() : waServiceBase<wnd_api, WndApiService>(wndApiServiceGuid) {}
  static const char *getServiceName() { return "Windowing API"; }
  virtual wnd_api *getService() { return wndApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_WND

#ifdef WASABI_COMPILE_WNDMGR

#include "bfc/api/api_wndmgr.h"

class WndMgrApiService : public waServiceBase<wndmgr_api, WndMgrApiService> {
public:
  WndMgrApiService() : waServiceBase<wndmgr_api, WndMgrApiService>(wndMgrApiServiceGuid) {}
  static const char *getServiceName() { return "Window Manager API"; }
  virtual wndmgr_api *getService() { return wndManagerApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_WNDMGR

#ifdef WASABI_COMPILE_SKIN

#include "bfc/api/api_skin.h"

class SkinApiService : public waServiceBase<api_skin, SkinApiService> {
public:
  SkinApiService() : waServiceBase<api_skin, SkinApiService>(skinApiServiceGuid) {}
  static const char *getServiceName() { return "Skinning API"; }
  virtual api_skin *getService() { return skinApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_SKIN

#ifdef WASABI_COMPILE_SCRIPT

#include "bfc/api/api_maki.h"

class MakiApiService : public waServiceBase<api_maki, MakiApiService> {
public:
  MakiApiService() : waServiceBase<api_maki, MakiApiService>(makiApiServiceGuid) {}
  static const char *getServiceName() { return "MAKI Scripting API"; }
  virtual api_maki *getService() { return makiApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_SCRIPT

#ifdef WASABI_COMPILE_MAKIDEBUG

#include "bfc/api/api_makidebug.h"

class MakiDebugApiService : public waServiceBase<api_makiDebugger, MakiDebugApiService> {
public:
  MakiDebugApiService() : waServiceBase<api_makiDebugger, MakiDebugApiService>(makiDebugApiServiceGuid) {}
  static const char *getServiceName() { return "MAKI Debugger API"; }
  virtual api_makiDebugger *getService() { return debugApi; }
  static FOURCC getServiceType() { return WaSvc::UNIQUE; }
};

#endif // WASABI_COMPILE_SKIN

class ApiInit {
public:
  static int init(HINSTANCE hinstance, GUID app_guid, const char *cmd_line, OSWINDOWHANDLE mainwnd = NULL); // 0 == error, 1 == success 
  static int shutdown(); // 0 == error, 1 == success 
  static int getInitCount(); // if you do init / shutdown more than once in the session, this may be useful to invalid some caches

private:
  #ifdef WASABI_COMPILE_SKIN
  #ifdef WASABI_COMPILE_COMPONENTS
  static waServiceFactoryI * groupcreate;
  #endif //WASABI_COMPILE_COMPONENTS
  #endif //WASABI_COMPILE_SKIN

  #ifdef WASABI_COMPILE_FONTS
  static waServiceFactoryI * win32Font;
  static FontApiService *fontapisvc;
  #endif //WASABI_COMPILE_FONTS

  #ifdef WASABI_COMPILE_SKIN
  static Widgets * widgets;
  static SkinApiService *skinapisvc;
  #endif //WASABI_COMPILE_SKIN

  static api_applicationI * app;
  static ApplicationApiService *appapisvc;

  static api_serviceI * svc;

  #ifdef WASABI_COMPILE_SYSCB
  static api_syscbI * syscb;
  static SysCBApiService *syscbapisvc;
  #endif

  #ifdef WASABI_COMPILE_MEMMGR
  static MemMgrApi * memmgr;
  static MemMgrApiService *memmgrapisvc;
  #endif

  #ifdef WASABI_COMPILE_CONFIG
  static api_configI * cfg;
  static ConfigApiService *cfgapisvc;
  #endif

  #ifdef WASABI_COMPILE_IMGLDR
  static ImgLdrApi * img;
  static ImgLdrApiService *imgldrapisvc;
  #endif

  #ifdef WASABI_COMPILE_FONTS
  static FontApi * font;
  #endif

  #ifdef WASABI_COMPILE_XMLPARSER
  static XmlApi * xml;
  static XmlApiService *xmlapisvc;
  #endif

  #ifdef WASABI_COMPILE_FILEREADER
  static FileReaderApi * fr;
  #ifdef WASABI_COMPILE_ZIPREADER
  static waServiceFactoryI * zipRead;
  #endif
  static waServiceFactoryI * resRead;
  static FileReaderApiService *frapisvc;
  #endif

  #ifdef WASABI_COMPILE_LOCALES
  static api_localesI *loc;
  static LocalesApiService *localesapisvc;
  #endif

  #ifdef WASABI_COMPILE_TIMERS
  static TimerApi * timer;
  static TimerApiService *timerapisvc;
  #endif

  #ifdef WASABI_COMPILE_WND
  static WndApi * wnd;
  static ForeignWnd * mainWnd;
  static WndApiService *wndapisvc;
  #endif

  #ifdef WASABI_COMPILE_WNDMGR
  static WndMgrApi * wndmgr;
  static WndMgrApiService *wndmgrapisvc;
  #endif

  #ifdef WASABI_COMPILE_COMPONENTS
  #endif

  #ifdef WASABI_COMPILE_METADB
  #endif

  #ifdef WASABI_COMPILE_SCRIPT
  static api_makiI * maki;
  static MakiApiService *makiapisvc;
  #endif

  #ifdef WASABI_COMPILE_SKIN
  static SkinApi * skin;
  #endif

  #ifdef WASABI_COMPILE_MAKIDEBUG
  static MakiDebuggerApi * debug;
  static MakiDebugApiService *makidebugapisvc;
  #endif

  #ifdef WASABI_COMPILE_MEDIACORE
  static CoreApiService *coreapisvc;
  #endif

  static int initcount;
};

#endif

