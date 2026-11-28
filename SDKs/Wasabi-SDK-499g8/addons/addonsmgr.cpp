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
#include "addonsmgr.h"
#include "xuiaddonslist.h"
#include "../studio/api.h"
#include "../studio/services/svc_filesel.h"
#include "../bfc/svc_enum.h"
#include "../bfc/msgbox.h"
#include "../common/metatags.h"
#include "../common/popup.h"
#include "../common/selectfile.h"
#include "../common/readdir.h"

AddOnsMgr addonsmgr;

// -----------------------------------------------------------------------
// {140DA035-EABA-447c-893D-F54672A0CE6E}
static const GUID addons_guid = 
{ 0x140da035, 0xeaba, 0x447c, { 0x89, 0x3d, 0xf5, 0x46, 0x72, 0xa0, 0xce, 0x6e } };

// -----------------------------------------------------------------------
AddOnsMgr::AddOnsMgr() : CfgItemI("AddOns/SkinMods", addons_guid) {
  setCfgXml("wasabi.config.addons");
}

// -----------------------------------------------------------------------
int AddOnsMgr::skincb_onGuiLoaded() {
  loadAll();
  return 1;
}

// -----------------------------------------------------------------------
AddOnsMgr::~AddOnsMgr() {
  for (int i=0;i<2;i++)
    addons[i].deleteAll();
}

// -----------------------------------------------------------------------
void AddOnsMgr::loadAll() {
  int i;
  for (i=0;i<2;i++)
    addons[i].deleteAll();
  char c[WA_MAX_PATH];
  for (int j=0;j<2;j++) {
    for (i=0;;i++) {
      StringPrintf d("%s_mod%d", j==0?"global":api->getSkinName(),i);
      WASABI_API_CONFIG->getStringPrivate(d, c, WA_MAX_PATH, "");
      if (!*c)
        break;
      insertMod(j,c,0);
    }
  }
}

// -----------------------------------------------------------------------
void AddOnsMgr::saveAll() {
  for (int j=0;j<2;j++) {
    int i;
    for (i=0;i<addons[j].getNumItems();i++) {
      StringPrintf d("%s_mod%d", j==0?"global":api->getSkinName(),i);
      WASABI_API_CONFIG->setStringPrivate(d, addons[j].enumItem(i)->getFilename());
    }
    
    WASABI_API_CONFIG->setStringPrivate(StringPrintf("%s_mod%d", j==0?"global":api->getSkinName(),i), "");
  }
}

// -----------------------------------------------------------------------
void AddOnsMgr::insertMod(int type, const char *file, int save) {
  ASSERT(type == TYPE_GLOBAL || type == TYPE_THISSKIN);

  AddOnReader *reader = new AddOnReader(file);
  if (!reader->gotData())
    api->messageBox(StringPrintf("%s does not appear to have a valid description block", file), "Error loading add-on", MSGBOX_OK, NULL, NULL);
  else {
    addons[type].addItem(reader);
    int skinpart = api->loadSkinFile(file);
    reader->setSkinPart(skinpart);
  }
  refreshAllLists();
  if (save)
    saveAll();
}

// -----------------------------------------------------------------------
void AddOnsMgr::removeMod(int skinpart) {
  int brk=0;
  for (int type=0;type<2&&!brk;type++) {
    for (int i=0;i<addons[type].getNumItems()&&!brk;i++) {
      AddOnInfo *inf = addons[type].enumItem(i);
      if (inf->getSkinPart() == skinpart) {
        addons[type].removeByPos(i);
        api->unloadSkinPart(skinpart);
        delete inf;
        brk=1;
      }
    }
  }
  refreshAllLists();
  saveAll();
}

// -----------------------------------------------------------------------
AddOnInfo *AddOnsMgr::enumEntry(int type, int n) {
  ASSERT(type == TYPE_GLOBAL || type == TYPE_THISSKIN);
  return addons[type].enumItem(n);
}

// -----------------------------------------------------------------------
AddOnInfo *AddOnsMgr::getFromSkinPart(int partid) {
  int brk=0;
  AddOnInfo *found=NULL;
  for (int type=0;type<2&&!brk;type++) {
    for (int i=0;i<addons[type].getNumItems()&&!brk;i++) {
      AddOnInfo *inf = addons[type].enumItem(i);
      if (inf->getSkinPart() == partid) {
        found = inf;
        brk=1;
      }
    }
  }
  return found;
}

// -----------------------------------------------------------------------
void AddOnsMgr::aboutEntry(int skinpart) {
  AddOnInfo *i = getFromSkinPart(skinpart);
  if (!i) return;

  String about;

  if (i->getFullName()) {
    if (i->gotVersion())
      about += StringPrintf("%s (version %s)\n\n", i->getFullName(), i->getVersion());
    else
      about += StringPrintf("%s\n\n", i->getFullName());
  }
  if (i->gotAuthor()) {
    about += StringPrintf("Author : %s", i->getAuthor());
    if (i->gotEmail())
      about += StringPrintf(" (%s)\n", i->getEmail());
  } else if (i->gotEmail()) {
      about += StringPrintf("%s\n", i->getEmail());
  }

  if (i->gotHomepage()) {
    about += StringPrintf("%s\n", i->getHomepage());
  }

  if (i->gotComment()) {
    about += "\n";
    about += i->getComment();
  }
  api->messageBox(about, "About This Add-On", MSGBOX_OK, NULL, NULL);
}

// -----------------------------------------------------------------------
void AddOnsMgr::refreshAllLists() { 
  foreach(lists) 
    refreshList(lists.getfor());
  endfor;
}

// -----------------------------------------------------------------------
void AddOnsMgr::refreshList(AddOnsList *list, int seemless) {
  int savedscrl=-1;
  int savedpos=-1;
  if (seemless) {
    savedscrl = list->getScrollY();
    savedpos = list->getItemFocused();
  }
  list->deleteAllItems();
  int type = list->getModsType();
  ASSERT(type == TYPE_GLOBAL || type == TYPE_THISSKIN);
  foreach(addons[type])
    AddOnInfo *info = addons[type].getfor();
    int p = list->addItem(info->getFullName(), info->getSkinPart());
    list->setSubItem(p, 0, info->getFilename());
  endfor;
  if (seemless) {
    if (savedscrl != -1) list->scrollToY(savedscrl);
    if (savedpos != -1) list->setSelected(savedpos, 1);
  }
}

// -----------------------------------------------------------------------
void AddOnsMgr::registerList(AddOnsList *l) {
  lists.addItem(l);
}

// -----------------------------------------------------------------------
void AddOnsMgr::unregisterList(AddOnsList *l) {
  lists.removeItem(l);
}

// -----------------------------------------------------------------------
void AddOnsMgr::add(int type, int x, int y) {
  String path = api->path_getAppPath();
  path.cat(DIRCHARSTR);
  path.cat("scripts");

  PopupMenu m;
  int pos=0;

  static const char *list[] = {
    "script.xml", "skinmod.xml", "addon.xml", NULL
  };

  ReadDir dir(path);
  while (dir.next()) {
    if (dir.isDir()) {
      for (int i = 0; list[i] != NULL; i++) {
        const char *p = list[i];
        StringPrintf scan("%s%s%s%s%s", path.getValue(), DIRCHARSTR, dir.getFilename(), DIRCHARSTR, p);
        if (FEXISTS(scan)) {
          m.addCommand(dir.getFilename(), pos++);
          break;
        }
      }
    }
  }

  if (!pos) m.addCommand("<no add-ons detected>",-1,0,1);

  m.addSeparator();

  m.addCommand("Browse...",pos);

  int rv=m.popAtXY(x, y);

  String loadfn;
  if (rv >= 0 && rv < pos) { // figure out what to load
    const char *t=m.getCommandText(rv);
    for (int i = 0; list[i] != NULL; i++) {
      const char *p = list[i];
      loadfn.printf("%s%s%s%s%s", path.getValue(), DIRCHARSTR, t, DIRCHARSTR, p);
      if (FEXISTS(loadfn)) break;
      loadfn = NULL;
    }
  } else if (rv==pos) {
    String extlist="Addons (*.xml)|*.xml|All files (*.*)|*.*|";
    extlist.changeChar('|', '\0');
    SelectFile sf(api->main_getRootWnd());
    sf.setIdent("addons");
    sf.runSelector("files", FALSE, extlist);
    if (sf.getNumFiles() > 0) loadfn = sf.enumFilename(0);
  }

  if (!loadfn.isempty()) {    
    insertMod(type, loadfn);
  } 
}
