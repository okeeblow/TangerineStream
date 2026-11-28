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

#include "stdsvc.h"

#include <studio/services/creators.h>	// WndCreateCreator

#include "actioner.h"
#include "stdaction.h"

#include <bfc/feeds/feedwatcherso.h>

static WACStdSvc StdSvc;
WAComponentClient *the = &StdSvc;

// StdSvc's GUID
// {935CE1FC-5C4E-4600-B857-D0D2E6B614C6}
static const GUID guid = 
{ 0x935ce1fc, 0x5c4e, 0x4600, { 0xb8, 0x57, 0xd0, 0xd2, 0xe6, 0xb6, 0x14, 0xc6 } };

#include <studio/services/svc_filesel.h>
#include <studio/services/servicei.h>

#include "opendir.h"
#include "openfile.h"
#include "openlocation.h"
#include "playersvc.h"
#include "cfgscriptobj.h"

WACStdSvc::WACStdSvc() : WAComponentClient("Standard services") {
  // open by file
  registerService(new waServiceFactoryT<svc_fileSelector, DefOpen>);
  // open by directory
  registerService(new waServiceFactoryT<svc_fileSelector, OpenDirectory>);
  // open location (or any hand-typed url)
  registerService(new waServiceFactoryT<svc_fileSelector, OpenLocation>);
  // eject button functionality
  registerService(new PlayerCreator<PlayerSvc>);
  // config script objects
  registerService(new ScriptObjectCreator<ConfigScriptObjectSvc>);
  // cmdline action mapper
  registerResource(new Actioner);

  // "EXIT" action
  registerService(new ExitAction);

  // weird huh :/
  registerService(new ScriptObjectCreator< ScriptObjectControllerCreator<FeedWatcherScriptController> >);
}

GUID WACStdSvc::getGUID() {
  return guid;
}
