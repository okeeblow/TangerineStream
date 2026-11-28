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

#include "xuiaddmusic.h"

#include <studio/api.h>
#include <common/choosedir.h>
#include <guiobjects/xuiquerylist.h>

#include "watcher.h"

char AddMusicXuiObjectStr[]="MediaLibrary:AddMusic";
char AddMusicXuiSvcName[]="Add Music Service";

#define ADDMUSIC_SETQUERYLIST 10
#define CB_SETQUERYLIST 20

AddMusic::AddMusic() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "querylist", ADDMUSIC_SETQUERYLIST, XUI_ATTRIBUTE_IMPLIED);
}

AddMusic::~AddMusic() {
  getGuiObject()->guiobject_removeAppCmds(this);
}

int AddMusic::onInit() {
  ADDMUSIC_PARENT::onInit();
  appcmds_addCmd("Add to Library", 0, AppCmds::SIDE_LEFT);
  getGuiObject()->guiobject_addAppCmds(this);
  if (!querylist_id.isempty())
    postDeferredCallback(CB_SETQUERYLIST, 0);
  return 1;
}

int AddMusic::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return ADDMUSIC_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);
  switch (xmlattributeid) {
    case ADDMUSIC_SETQUERYLIST:
      setXuiQueryList(value);
    break;
    default:
      return 0;
  }
  return 1;
}

void AddMusic::setXuiQueryList(const char *v) {
  if (!isInited()) {
    querylist_id = v;
    return;
  }
  ScriptObject *o = WASABI_API_MAKI->maki_findObject(v);
  if (!o) return;
  ScriptQueryList *querylist = static_cast<ScriptQueryList *>(o->vcpu_getInterface(queryListGuid));
  if (!querylist) return;
  sqs_setMultiQueryServer(querylist);
}

int AddMusic::onDeferredCallback(int p1, int p2) {
  switch (p1) {
    case CB_SETQUERYLIST:
      setXuiQueryList(querylist_id);
      querylist_id = NULL;
      break;
    default:
      return ADDMUSIC_PARENT::onDeferredCallback(p1, p2);
  }
  return 1;
}

void AddMusic::appcmds_onCommand(int id, const RECT *buttonRect, int which_btn) {
  switch (id) {
    case 0: {
#if 0
      SelectFile sf(this, "Add ", "...");
      sf.setPopPosition(buttonRect->left, buttonRect->bottom);
      sf.runSelector(NULL, TRUE, api->core_getExtSupportedExtensions());
      if (!sf.getNumFiles()) return;
      int nfiles = sf.getNumFiles();
      for (int i = 0; i < nfiles; i++) {
        const char *fn = sf.enumFilename(i);
        api->metadb_addNewItem(fn);
      }
      String amt;
      if (nfiles == 1) amt = "1 item found";
      else amt.printf("%d items found", nfiles);
      getGuiObject()->guiobject_setStatusText(amt, TRUE);
      sqs_setQuery("", SetQuery::FORWARD, 1);
#else
      ChooseDir cd(this);
      if (cd.choose("Select directory"))
        Watcher::addDir(cd.getResult());
#endif
    }
    break;
  }
}
