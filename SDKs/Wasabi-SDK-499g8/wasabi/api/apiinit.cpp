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
#include "studio/api.h"

//<?#include "<class data="implementationheader"/>"
#include "apiinit.h"
//?>

#define WASABI_SAYMYNAME
#include <wasabicfg.h>
#undef WASABI_SAYMYNAME

#include "apiinit.h"

#ifdef WASABI_COMPILE_IMGLDR_PNGREAD
#include "pngload/loader.h"
#endif

#include "studio/services/servicei.h"

#ifdef WASABI_COMPILE_LOCALES
#include <bfc/api/api_localesi.h>
#endif

#ifdef WASABI_COMPILE_CONFIG
#include <bfc/api/api_configi.h>
#include <bfc/attribs/cfgitem.h>
#endif
#ifdef WASABI_COMPILE_FONTS
#include "fontapi.h"
#endif
#ifdef WASABI_COMPILE_SKIN
#include "skinapi.h"
#include <studio/groupwndcreate.h>
#endif
#ifdef WASABI_COMPILE_SCRIPT
#include "makiapi.h"
#include <studio/script/objecttable.h>
#endif
#ifdef WASABI_COMPILE_MEMMGR
#include "memmgrapi.h"
#endif
#ifdef WASABI_COMPILE_TIMERS
#include "timerapi.h"
#endif
#ifdef WASABI_COMPILE_WND
#include "wndapi.h"
#include <bfc/wnds/foreignwnd.h>
#endif
#ifdef WASABI_COMPILE_XMLPARSER
#include "xmlapi.h"
#endif
#ifdef WASABI_COMPILE_IMGLDR
#include "imgldrapi.h"
#endif
#ifdef WASABI_COMPILE_FONTS
#include <bfc/platform/win32/truetypefont_win32.h>
#endif
#ifdef WASABI_COMPILE_WNDMGR
#include "wndmgrapi.h"
#endif
#ifdef WASABI_COMPILE_MAKIDEBUG
#include "debugapi.h"
#endif
#ifdef WASABI_COMPILE_MAKIDEBUG
#include <bfc/api/api_core.h>
#endif
#ifdef WASABI_COMPILE_FILEREADER
#include "filereaderapi.h"
#ifdef WASABI_COMPILE_ZIPREADER
#include <studio/zipread.h>
#endif
#include <studio/resread.h>
#endif

#if defined(WASABI_COMPILE_COMPONENTS) | defined(GEN_FF) // MULTIAPI-FIXME
#include <studio/compon.h>
#endif

#ifdef WASABI_COMPILE_WNDMGR
#include <studio/gc.h>
#endif

#ifndef WASABI_CUSTOM_MODULE_SVCMGR
DECLARE_MODULE_SVCMGR;
#endif

int ApiInit::init(HINSTANCE hinstance, GUID app_guid, const char *cmd_line, OSWINDOWHANDLE mainwnd) {

  initcount++;

  #ifndef WA3COMPATIBILITY
  
  // ------------------------------------------------------------------------
  // LIBRARY INITIALIZATION
  // ------------------------------------------------------------------------
  app = new api_applicationI(hinstance);
  applicationApi = app;

  app->setCommandLine(cmd_line);
  app->setGUID(app_guid);

  // ------------------------------------------------------------------------
  // Init services
  // ------------------------------------------------------------------------
  svc = new api_serviceI();
  serviceApi = svc;

  svc->service_register(appapisvc = new ApplicationApiService());

  // ------------------------------------------------------------------------
  // Init callback system
  // ------------------------------------------------------------------------
  #ifdef WASABI_COMPILE_SYSCB
  syscb = new api_syscbI();
  sysCallbackApi = syscb;

  svc->service_register(syscbapisvc = new SysCBApiService());
  #endif

  #ifdef WASABI_COMPILE_MEMMGR
  // ------------------------------------------------------------------------
  // Init centralized memory manager api
  // ------------------------------------------------------------------------
  memmgr = new MemMgrApi();
  memmgrApi = memmgr;

  svc->service_register(memmgrapisvc = new MemMgrApiService());
  #endif

  #ifdef WASABI_COMPILE_FILEREADER
  // ------------------------------------------------------------------------
  // Init file reader api and optional file readers 
  // ------------------------------------------------------------------------
  fr = new FileReaderApi();
  fileApi = fr;
  svc->service_register(frapisvc = new FileReaderApiService());
  //WASABI_API_SVC->service_register(&diskReader);
  // set up custom file readers
#ifdef WASABI_COMPILE_ZIPREADER
  zipRead = new waServiceTSingle<svc_fileReader, ZipRead>;
#endif
  resRead = new waServiceTSingle<svc_fileReader, ResRead>;
#ifdef WASABI_COMPILE_ZIPREADER
  WASABI_API_SVC->service_register(zipRead);
#endif
  WASABI_API_SVC->service_register(resRead);
  #endif // WASABI_COMPILE_FILEREADER

  #ifdef WASABI_COMPILE_CONFIG
  // ------------------------------------------------------------------------
  // Init config api and register optional config items
  // ------------------------------------------------------------------------
  cfg = new api_configI();
  configApi = cfg;
  svc->service_register(cfgapisvc = new ConfigApiService());
  // set up custom options
  //WASABI_API_CONFIG->config_registerCfgItem(&options);
  #endif // WASABI_COMPILE_CONFIG

  MODULE_SVCMGR_ONINIT2();

  #ifdef WASABI_COMPILE_IMGLDR
  // ------------------------------------------------------------------------
  // Init image loading api and optional image formats
  // ------------------------------------------------------------------------
  img = new ImgLdrApi();
  imgLoaderApi = img;
  svc->service_register(imgldrapisvc = new ImgLdrApiService());

  #endif

  #ifdef WASABI_COMPILE_FONTS
  // ------------------------------------------------------------------------
  // Init font rendering api
  // ------------------------------------------------------------------------
  font = new FontApi();
  fontApi = font;
  win32Font = new FontCreator<TrueTypeFont_Win32>;
  WASABI_API_SVC->service_register(win32Font);
  svc->service_register(fontapisvc = new FontApiService());
  #endif

  #ifdef WASABI_COMPILE_XMLPARSER
  // ------------------------------------------------------------------------
  // Init xml api
  // ------------------------------------------------------------------------
  xml = new XmlApi();
  xmlApi = xml;
  svc->service_register(xmlapisvc = new XmlApiService());
  #endif

  #ifdef WASABI_COMPILE_LOCALES
  // ------------------------------------------------------------------------
  // Init localization
  // ------------------------------------------------------------------------
  loc = new api_localesI();
  localesApi = loc;
  svc->service_register(localesapisvc = new LocalesApiService());
  #endif

  #ifdef WASABI_COMPILE_TIMERS
  // ------------------------------------------------------------------------
  // Init timer api
  // ------------------------------------------------------------------------
  timer = new TimerApi();
  timerApi = timer;
  svc->service_register(timerapisvc = new TimerApiService());
  #endif

  #ifdef WASABI_COMPILE_WND
  // ------------------------------------------------------------------------
  // Init window api
  // ------------------------------------------------------------------------
  wnd = new WndApi();
  wndApi = wnd;
  if (mainwnd != NULL) {
    mainWnd = new ForeignWnd(mainwnd, WASABI_API_APP->main_gethInstance());
    DebugString("main wnd handle is %x\n", mainWnd->gethWnd());
    wnd->main_setRootWnd(mainWnd);
  } else {
    wnd->main_setRootWnd(timer->genwnd);
  }
  svc->service_register(wndapisvc = new WndApiService());
  #endif

  // ------------------------------------------------------------------------
  // Init media core api
  // ------------------------------------------------------------------------
  #ifdef WASABI_COMPILE_MEDIACORE
  #ifndef WA3COMPATIBILITY
  #ifdef WASABI_CUSTOMIMPL_MEDIACORE
  coreApi = createCustomCoreApi();
  #else
  #error "not done yet"
  #endif
  #endif
  svc->service_register(coreapisvc = new CoreApiService());
  #endif

  #ifdef WASABI_COMPILE_WNDMGR
  // ------------------------------------------------------------------------
  // Init window manager api
  // ------------------------------------------------------------------------
  wndmgr = new WndMgrApi ();
  wndManagerApi = wndmgr;
  svc->service_register(wndmgrapisvc = new WndMgrApiService());
  #endif


  #ifdef WASABI_COMPILE_COMPONENTS
  // ------------------------------------------------------------------------
  // Init component api
  // ------------------------------------------------------------------------
  // ComponentApi *comp = new ComponentApi ();
  // componentApi = comp;
  #endif

  #ifdef WASABI_COMPILE_METADB
  // ------------------------------------------------------------------------
  // Init metadb api
  // ------------------------------------------------------------------------
  // MetaDbApi *metadb = new MetaDbApi ();
  // metadbApi = metadb;
  #endif

  #ifdef WASABI_COMPILE_SKIN
  // ------------------------------------------------------------------------
  // Init skin
  // ------------------------------------------------------------------------
  skin = new SkinApi();
  skinApi = skin;

  #ifdef WASABI_COMPILE_COMPONENTS
  groupcreate = new waServiceTSingle<svc_windowCreate, GroupWndCreateSvc>;
  WASABI_API_SVC->service_register(groupcreate);
  #endif  

  svc->service_register(skinapisvc = new SkinApiService());

  #endif

  #ifdef WASABI_COMPILE_MAKIDEBUG
  // ------------------------------------------------------------------------
  // Init script debugger api
  // ------------------------------------------------------------------------
  debug = new MakiDebuggerApi();
  debugApi = debug;
  svc->service_register(makidebugapisvc = new MakiDebugApiService());
  #endif

  #ifdef WASABI_COMPILE_SKIN
  widgets = new Widgets();
  #endif

  #endif

  MODULE_SVCMGR_ONINIT();

  #ifndef WA3COMPATIBILITY

  #ifdef WASABI_COMPILE_SCRIPT
  // ------------------------------------------------------------------------
  // Init scripting api
  // ------------------------------------------------------------------------
  maki = new api_makiI();
  makiApi = maki;
  maki->init();

  svc->service_register(makiapisvc = new MakiApiService());

  #ifdef WASABI_COMPILE_WNDMGR
  garbageCollector = new GarbageCollector();
  #endif

  ObjectTable::start();
  #endif

  #endif //wa3compat

  return 1; // success
}

int ApiInit::shutdown() {
  #ifndef WA3COMPATIBILITY

  #ifdef WASABI_COMPILE_WNDMGR
  garbageCollector->gccb_onGarbageCollect();
  delete garbageCollector;
  #endif

  #ifdef WASABI_COMPILE_SKIN
  skin->preShutdown();
  #endif

  MODULE_SVCMGR_ONSHUTDOWN();

#if defined(WASABI_COMPILE_COMPONENTS) | defined(GEN_FF) // MULTIAPI-FIXME
  ComponentManager::unloadAll();
#endif

  #ifdef WASABI_COMPILE_SKIN
  delete skin;
  skinApi = NULL;
  delete skinapisvc;
  #endif

  #ifdef WASABI_COMPILE_SCRIPT
  ObjectTable::shutdown();
  #endif

  #ifdef WASABI_COMPILE_SKIN
  delete widgets;
  widgets = NULL;
  #endif

  #ifdef WASABI_COMPILE_SCRIPT
  delete maki;
  makiApi = NULL;
  delete makiapisvc;
  #endif
  
  #ifdef WASABI_COMPILE_COMPONENTS
  WASABI_API_SVC->service_deregister(groupcreate);
  delete groupcreate;
  #endif 
  
  #ifdef WASABI_COMPILE_FONTS
  WASABI_API_SVC->service_deregister(win32Font);
  delete win32Font;
  #endif
  
  #ifdef WASABI_COMPILE_MEDIACORE
  #ifndef WA3COMPATIBILITY
  #ifdef WASABI_CUSTOMIMPL_MEDIACORE
  destroyCustomCoreApi(coreApi);
  #else
  #error "not done yet"
  #endif
  #endif
  delete coreapisvc;
  #endif

  #ifdef WASABI_COMPILE_METADB
  #endif

  #ifdef WASABI_COMPILE_COMPONENTS
  #endif

  #ifdef WASABI_COMPILE_WNDMGR
  delete wndmgr;
  wndManagerApi = NULL;
  delete wndmgrapisvc;
  #endif

  #ifdef WASABI_COMPILE_TIMERS
  delete timer;
  timerApi = NULL;
  delete timerapisvc;
  #endif

  #ifdef WASABI_COMPILE_LOCALES
  delete loc;
  localesApi = NULL;
  delete localesapisvc;
  #endif

  #ifdef WASABI_COMPILE_FILEREADER
#ifdef WASABI_COMPILE_ZIPREADER
  WASABI_API_SVC->service_deregister(zipRead);
  delete zipRead;
#endif
  WASABI_API_SVC->service_deregister(resRead);
  delete resRead;
  delete fr;
  fileApi = NULL;
  delete frapisvc;
  #endif 

  #endif

  #ifdef WASABI_COMPILE_WND
  delete mainWnd;
  mainWnd = NULL;
  delete wnd;
  wndApi = NULL;
  delete wndapisvc;
  #endif

  #ifdef WASABI_COMPILE_FONTS
  delete font;
  fontApi = NULL;
  delete fontapisvc;
  #endif

  #ifndef WA3COMPATIBILITY

  #ifdef WASABI_COMPILE_XMLPARSER
  delete xml;
  xmlApi = NULL;
  delete xmlapisvc;
  #endif

  #ifdef WASABI_COMPILE_IMGLDR
  delete img;
  imgLoaderApi = NULL;
  delete imgldrapisvc;
  #endif

  #ifdef WASABI_COMPILE_CONFIG
  delete cfg;
  configApi = NULL;
  delete cfgapisvc;
  #endif 

  MODULE_SVCMGR_ONSHUTDOWN2();

  #ifdef WASABI_COMPILE_MEMMGR
  delete memmgr;
  memmgrApi = NULL;
  delete memmgrapisvc;
  #endif

  #ifdef WASABI_COMPILE_SYSCB
  delete syscb;
  sysCallbackApi = NULL;
  delete syscbapisvc;
  #endif

  delete appapisvc;

  delete svc;
  serviceApi = NULL;

  delete app;

  #endif //wa3compatibility

  #ifdef WASABI_COMPILE_MAKIDEBUG
  delete debug;
  debugApi = NULL;
  delete makidebugapisvc;
  #endif

  return 1; // success
}

int ApiInit::getInitCount() {
  return initcount;
}

#ifdef WASABI_COMPILE_SKIN
#ifdef WASABI_COMPILE_COMPONENTS
waServiceFactoryI * ApiInit::groupcreate = NULL;
#endif
#endif //WASABI_COMPILE_SKIN

#ifdef WASABI_COMPILE_FONTS
waServiceFactoryI * ApiInit::win32Font = NULL;
FontApiService *ApiInit::fontapisvc = NULL;
#endif //WASABI_COMPILE_FONTS

#ifdef WASABI_COMPILE_SKIN
Widgets * ApiInit::widgets = NULL;
SkinApiService *ApiInit::skinapisvc = NULL;
#endif

api_applicationI * ApiInit::app = NULL;
ApplicationApiService *ApiInit::appapisvc = NULL;

api_serviceI * ApiInit::svc = NULL;

#ifdef WASABI_COMPILE_SYSCB
api_syscbI * ApiInit::syscb = NULL;
SysCBApiService *ApiInit::syscbapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_MEMMGR
MemMgrApi * ApiInit::memmgr = NULL;
MemMgrApiService *ApiInit::memmgrapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_CONFIG
api_configI * ApiInit::cfg = NULL;
ConfigApiService *ApiInit::cfgapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_IMGLDR
ImgLdrApi * ApiInit::img = NULL;
ImgLdrApiService *ApiInit::imgldrapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_FONTS
FontApi * ApiInit::font = NULL;
#endif

#ifdef WASABI_COMPILE_XMLPARSER
XmlApi * ApiInit::xml = NULL;
XmlApiService *ApiInit::xmlapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_FILEREADER
FileReaderApi * ApiInit::fr = NULL;
#ifdef WASABI_COMPILE_ZIPREADER
waServiceFactoryI * ApiInit::zipRead = NULL;
#endif
waServiceFactoryI * ApiInit::resRead = NULL;
FileReaderApiService *ApiInit::frapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_LOCALES
api_localesI * ApiInit::loc = NULL;
LocalesApiService *ApiInit::localesapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_TIMERS
TimerApi * ApiInit::timer = NULL;
TimerApiService *ApiInit::timerapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_WND
WndApi * ApiInit::wnd = NULL;
ForeignWnd * ApiInit::mainWnd = NULL;
WndApiService *ApiInit::wndapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_WNDMGR
WndMgrApi * ApiInit::wndmgr = NULL;
WndMgrApiService *ApiInit::wndmgrapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_COMPONENTS
#endif

#ifdef WASABI_COMPILE_METADB
#endif

#ifdef WASABI_COMPILE_SCRIPT
api_makiI * ApiInit::maki = NULL;
MakiApiService *ApiInit::makiapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_SKIN
SkinApi * ApiInit::skin = NULL;
#endif

#ifdef WASABI_COMPILE_MAKIDEBUG
MakiDebuggerApi * ApiInit::debug = NULL;
MakiDebugApiService *ApiInit::makidebugapisvc = NULL;
#endif

#ifdef WASABI_COMPILE_MEDIACORE
CoreApiService *ApiInit::coreapisvc = NULL;
#endif

int ApiInit::initcount = 0;