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
#include "shoutcastwac.h"
#include "shoutcast.h"

static WACNAME wac;
WAComponentClient *the = &wac;
WACNAME *shoutcastwac = &wac;

#include "../studio/services/creators.h"
#include "../studio/services/servicei.h"
#include "wndcreatesvc.h"
#include "shoutcastwnd.h"
#include "server.h"
#include "nosound.h"

// {3928F3D6-C764-4369-AB82-50F1454AC29B}
static const GUID guid = 
{ 0x3928f3d6, 0xc764, 0x4369, { 0xab, 0x82, 0x50, 0xf1, 0x45, 0x4a, 0xc2, 0x9b } };

_int graphtime("Listeners graph update interval (in seconds)",30);
_int retrydelay("Connections retry delay (in seconds)",20);

WACNAME::WACNAME() : WAComponentClient("Shoutcaster v" SC_VERSION) {
#ifdef FORTIFY
  FortifySetName("shoutcast.wac");
  FortifyEnterScope();
#endif
  registerService(new MediaConverterCreator<Shoutcast>);
  registerService(new MediaConverterCreator<NoSound>);
  registerService(new WndCreateCreator<ShoutcastWndCreateSvc>);
  registerSkinFile("xml/serverinfo.xml");
  JNL::open_socketlib();
}

WACNAME::~WACNAME() {
#ifdef FORTIFY
  FortifyLeaveScope();
#endif
}

void WACNAME::onDestroy() {
  saveConfig();
  g_servers.deleteAll();
  JNL::close_socketlib();
}

GUID WACNAME::getGUID() {
  return guid;
}

void WACNAME::onRegisterServices() {
  registerAttribute(&graphtime);
  registerAttribute(&retrydelay);
  
  loadConfig();
}

RootWnd *WACNAME::createWindow(int n, RootWnd *parentWnd) {
  ASSERT(n == 0);
  shoutcastwnd=new ShoutcastWnd();
  shoutcastwnd->init(parentWnd);
  //shoutcastwnd->setParent(parentWnd);
  return shoutcastwnd;
}

void WACNAME::onCreate() {
  api->autopopup_registerGuid(getGUID(), getName());
}

int WACNAME::onCommand(const char *cmd, int param1, int param2, void *ptr, int ptrlen) {
  if(!STRICMP(cmd,"add")) {
    Server *s = new Server();
    g_servers.addItem(s);
    g_serverlistwnd->refresh();
    s->init();
  }
  return 0;
}

void WACNAME::loadConfig() {
  // load config
  int n=WASABI_API_CONFIG->getIntPrivate("nb_servers",0);
  for(int i=0;i<n;i++) {
    Server *s=new Server;
    g_servers.addItem(s);
    s->loadConfig(StringPrintf("servers/%i/",i));
    s->init();
  }
}

void WACNAME::saveConfig() {
  int n=g_servers.getNumItems();
  WASABI_API_CONFIG->setIntPrivate("nb_servers",n);
  for(int i=0;i<n;i++) g_servers[i]->saveConfig(StringPrintf("servers/%i/",i));
}
