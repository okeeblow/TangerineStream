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

#include "infoedit.h"
#include "resource.h"

#include <bfc/wnds/blankwnd.h>
#include <common/checkwnd.h>
#include <bfc/wnds/textbar.h>

#include <common/metatags.h>

#include <studio/api.h>

#include <common/editwnd.h>
#include <bfc/wnds/buttwnd.h>
#include <bfc/msgbox.h>

#include <core/corehandle.h>

#include "control.h"

// Window creation services includes.
#include <studio/services/creators.h>
#include <studio/services/servicei.h>
#include <studio/services/svc_contextcmd.h>
#include <studio/services/svc_droptarget.h>
#include <common/contextcmd.h>
#include <common/filename.h>
#include <bfc/wndcreator.h>

#include <preferences/preferences.h>

static WACinfoedit infoedit;
WAComponentClient *the = &infoedit;
WACinfoedit *infoeditwac = &infoedit;

RecycleString current;

// {6A17D510-7833-4cec-A420-F08F4B280782}
static const GUID guid = 
{ 0x6a17d510, 0x7833, 0x4cec, { 0xa4, 0x20, 0xf0, 0x8f, 0x4b, 0x28, 0x7, 0x82 } };

class CvtContextCmd : public ContextCmdT<Filename> {
	enum {
    COMMAND_EDIT_NONE,
    COMMAND_EDIT_FILEINFOS,
  };
public:
	CvtContextCmd() : ContextCmdT<Filename>()
	{
		addCommandById("View file info",COMMAND_EDIT_FILEINFOS);
	}

	virtual void onCommand(Filename * item,int n,int cmdid)
	{
		switch(cmdid)
		{
		case COMMAND_EDIT_FILEINFOS:
      current=item->getFilename();
      if(infownd) infownd->updateInfos();
      else if(!api->skinwnd_enumByGuid(guid,0)) {
        api->skinwnd_createByGuid(guid);
      }
			break;
		}
	}
};

_bool cfg_auto_upd("Auto-update to current playing file",false);
_bool cfg_close("Close window on update",false);

class InfoDrop : public svc_dropTargetI {
public:
  static const char *getServiceName() { return "Infoedit drop target"; }

  virtual int testTarget(FOURCC type) { return (type == 'info'); }
  virtual DragInterface *getDragInterfaceForType(FOURCC type) {
    if (testTarget(type)) return infownd;
    else return NULL;
  }
  virtual int releaseDragInterface(DragInterface *di) {
    return 1;
  }
};


WACinfoedit::WACinfoedit() : WAComponentClient("Media Info Editor") {

  // main menu entry
  registerAutoPopup(getGUID(), getName());

  // main window
  registerService(new WndCreateCreator< CreateWndByGuid<InfoWnd> >);

  // rightclick command
	registerService(new ContextCmdCreator<CvtContextCmd>);

  // drag&drop on xml group
  registerService(new DropTargetCreator<InfoDrop>);

  registerAttribute(&cfg_auto_upd);
  registerAttribute(&cfg_close);

  registerSkinFile("xml/config.xml"); 
}

WACinfoedit::~WACinfoedit() {
}

GUID WACinfoedit::getGUID() {
  return guid;
}

void WACinfoedit::onCreate() {
  CoreHandle().addCallback(this);

  // {81F5224A-59B6-4a45-9E93-87079CAD588F}
  static const GUID prefsguid = 
  { 0x81f5224a, 0x59b6, 0x4a45, { 0x9e, 0x93, 0x87, 0x7, 0x9c, 0xad, 0x58, 0x8f } };
  api->preferences_registerGroup("wasabi.infoedit.config","Media Info Editor",prefsguid);
}

void WACinfoedit::onDestroy() {
  CoreHandle().delCallback(this);
}

int WACinfoedit::corecb_onTitleChange(const char *title) {
  int upd=current.isempty();
  current=api->core_getCurrent(0);
  if(upd || cfg_auto_upd) {
    if(infownd) infownd->updateInfos();
  }
  return 0;
}

int WACinfoedit::onCommand(const char *cmd, int param1, int param2, void *ptr, int ptrlen) {
  if(STRCASEEQL(cmd,"display")) {
    if(!current.isempty()) {
      api->skinwnd_toggleByGuid(guid);
      return 1;
    }
  }
  if(STRCASEEQL(cmd,"candisplay")) return !current.isempty();
  if(STRCASEEQL(cmd,"displayfile")) {
    current=(const char *)ptr;
    if(infownd) infownd->updateInfos();
    else if(!api->skinwnd_enumByGuid(guid,0)) api->skinwnd_createByGuid(guid);
    return 1;
  }
  return 0;
}
