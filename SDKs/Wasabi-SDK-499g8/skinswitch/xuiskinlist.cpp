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
#include "xuiskinlist.h"
#include "xuiskinshot.h"
#include "../common/script/c_script/c_text.h"
#include "../bfc/canvas.h"
#include "../studio/api.h"
#include "../bfc/msgbox.h"
#include "../common/readdir.h"

#define DC_SKINLIST_SELECT 0x6976

// -----------------------------------------------------------------------
char SkinListXuiObjectStr[] = "SkinSwitch:List"; // This is the xml tag
char SkinListXuiSvcName[] = "SkinSwitch:List XuiObject Service";

PtrList<cSkinInfo> cinfolist; //custom info holder

// -----------------------------------------------------------------------
SkinList::SkinList() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "target", SKINLIST_SETTARGET, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "infotarget", SKINLIST_SETINFOTARGET, XUI_ATTRIBUTE_IMPLIED);
  listInited = 0;
  setAutoCollapse(0);
  setVirtual(0);
}

SkinList::~SkinList() {
}

// -----------------------------------------------------------------------
int SkinList::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SKINLIST_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case SKINLIST_SETTARGET:
      setTarget(value);
      break;
    case SKINLIST_SETINFOTARGET:
      setInfoTarget(value);
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
int SkinList::onInit() {
  int t = SKINLIST_PARENT::onInit();
  refreshSkinList();
  return t;
}

// -----------------------------------------------------------------------
void SkinList::setTarget(const char *elementname) {
  target_id = elementname;
}

// -----------------------------------------------------------------------
void SkinList::setInfoTarget(const char *elementname) {
  infotarget_id = elementname;
}

// -----------------------------------------------------------------------
void SkinList::refreshSkinList() {
  deleteAllItems();
  skinlist.removeAll();
//  itemlist->addTreeItem(new SwitchTreeItem(new SkinItem("Default")), NULL, FALSE, FALSE);
  curSkin = api->getSkinName();
  selitem=NULL;

  ReadDir dir(api->getSkinsPath());
  while (dir.next()) {
    const char *fn = dir.getFilename(), *ext = Std::extension(fn);
    if (dir.isDir() || !STRICMP(ext, "wal") || !STRICMP(ext, "wsz") ||
        !STRICMP(ext, "zip")) {
      String fullfilename = fn;
      // if its a file then strip extension
      if (!dir.isDir()) {
        char *p=(char *)Std::extension(fd.cFileName);
        if(p && *p) *(p-1)=0;
      }
//        if(STRICMP(fd.cFileName,"Default")) {
        SkinInfosXmlReader *r = new SkinInfosXmlReader(fn, fullfilename);
        SwitchTreeItem *t = new SwitchTreeItem(r);
        skinlist.addItem(t);
        addTreeItem(t, NULL, FALSE, FALSE);
        if(!STRICMP(fn, curSkin)) selitem=t;
//        }
    }
  }

  for(int i=0;i<skinlist.getNumItems();i++) {
    SkinItem *item=skinlist[i]->getParam();
    if(item->getParentSkin()) {
      // find its parent
      for(int j=0;j<skinlist.getNumItems();j++) {
        if(!STRICMP(item->getParentSkin(),skinlist[j]->getParam()->getName())) {
          // move item
          moveTreeItem(skinlist[i], skinlist[j]);
          expandItem(skinlist[j]);   
        }
      }
    }
  }
  listInited=1;
  postDeferredCallback(DC_SKINLIST_SELECT, 0);
}

int SkinList::onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *src) {
  if (STRCASEEQLSAFE(action, "SWITCH")) {
    if (getCurItem()) {
      SkinItem *skinitem = (static_cast<SwitchTreeItem *>(getCurItem()))->getParam();
      api->skin_switchSkin((char *)skinitem->getName());
      return 1;
    }
  }
  if (STRCASEEQLSAFE(action, "REMOVE")) {
    if (getCurItem()) {
      // delete the skin if not current one
      SkinItem *skinitem = (static_cast<SwitchTreeItem *>(getCurItem()))->getParam();
      if(STRCASEEQL(api->getSkinName(),skinitem->getName())) return 0;
      if(api->messageBox(StringPrintf("Are you sure you want to delete this skin? (%s)",skinitem->getFullName()),"Warning",MSGBOX_OK|MSGBOX_NO,NULL,this)==MSGBOX_OK) {
        StringPrintf del("%s%s",api->getSkinsPath(),skinitem->getFilename());
        if (Std::isDirectory(del))
          Std::removeDirectory(del,1); // remove if it's a directory
        else
          FDELETE(del, FALSE); // remove if it's a file
        refreshSkinList();
      }
      return 1;
    }
  }
  return SKINLIST_PARENT::onAction(action, param, x, y, p1, p2, data, datalen, src);
}

int SkinList::onDeferredCallback(int p1, int p2) {
  if (p1 == DC_SKINLIST_SELECT) {
    if(selitem) selectItem(selitem);
  }
  return SKINLIST_PARENT::onDeferredCallback(p1, p2);
}

void SkinList::onItemSelected(TreeItem *item) {
  SkinItem *skinitem = (static_cast<SwitchTreeItem *>(item))->getParam();
  SkinInfosXmlReader *sixr = (static_cast<SkinInfosXmlReader *>(skinitem));
  sixr->load();

  GuiObject *tgo = getGuiObject()->guiobject_findObject(target_id);
  if (tgo != NULL) {
    XuiSkinShot *xss = static_cast<XuiSkinShot *>(tgo->guiobject_getScriptObject()->vcpu_getInterface(skinshotGuid));
    if (xss != NULL) // if null, then that's not an object we can use
      xss->setSkinItem(skinitem);
  }

  ScriptObject *itso = WASABI_API_MAKI->maki_findObject(infotarget_id); // FG>SUPERFUCKO! //getGuiObject()->guiobject_findObject(infotarget_id);
  if (itso != NULL) {
    C_Text txt(itso);
    String s;
    s = skinitem->getName();
    if (skinitem->getVersion()) {
      s += " ";
      s += skinitem->getVersion();
    }
    s += "\n";
    if (skinitem->getAuthor()) {
      s += "By ";
      s += skinitem->getAuthor();
      s += "\n";
    }
    s += "\n";
    if (skinitem->getCoder()) { //2 new fixed fields
      s += "Code By ";
      s += skinitem->getCoder();
      s += "\n";
    }
    if (skinitem->getArtist()) { //2 new fixed fields
      s += "Graphics By ";
      s += skinitem->getArtist();
      s += "\n";
    }
    if (skinitem->getComment()) {
      s += skinitem->getComment();
      s += "\n\n";
    }
    if (skinitem->getHomepage()) {
      s += skinitem->getHomepage();
      s += "\n";
    }
    if (skinitem->getEmail()) {
      s += skinitem->getEmail();
      s += "\n\n";
    }

    //show custom info
    int numofc = cinfolist.getNumItems();
    if (numofc > 0) {
      for (int xi=0; xi<numofc; xi++) {
        s += cinfolist.enumItem(xi)->getName().getValueSafe();
        s += ": ";
        s += cinfolist.enumItem(xi)->getData().getValueSafe();
        s += "\n";
      }
    }
    txt.setText(s);
  }
}

int SkinList::onLeftButtonDblClk(int x, int y) {
  TreeItem *item=getCurItem();
  if(!item) return 0;
  SkinItem *skinitem = (static_cast<SwitchTreeItem *>(item))->getParam();
  api->skin_switchSkin((char *)skinitem->getName());
  return 1;
}


SkinInfosXmlReader::SkinInfosXmlReader(const char *skinname, const char *fullname) : SkinItem(skinname,fullname) {
  file = skinname;
  loaded = 0;
}

void SkinInfosXmlReader::load() {
  if (loaded) return;
  // back compat
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/SkinInfo",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/SkinInfo/*",this);

  WASABI_API_XML->xmlreader_registerCallback("WasabiXML",this);
  WASABI_API_XML->xmlreader_registerCallback("WasabiXML/SkinInfo",this);
  WASABI_API_XML->xmlreader_registerCallback("WasabiXML/SkinInfo/*",this);

  WASABI_API_XML->xmlreader_loadFile(StringPrintf("%s%s/skin.xml",api->getSkinsPath(),file.v()),this);
  WASABI_API_XML->xmlreader_unregisterCallback(this);
}

void SkinInfosXmlReader::xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params) {
  if(!STRCMP(xmltag,"WinampAbstractionLayer") || !STRCMP(xmltag, "WasabiXML")) { 
    walversion=params->getItemValue("version");
  } else if(!STRCMP(xmltag,"cinfo")) { //get custom info
    cinfolist.addItem(new cSkinInfo(params->getItemValue("name"), params->getItemValue("data")));
  }
}

void SkinInfosXmlReader::xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *s) {
  if(!STRCMP(xmltag,"name")) fullname=s;
  else if(!STRCMP(xmltag,"version")) version=s;
  else if(!STRCMP(xmltag,"comment")) comment=s;
  else if(!STRCMP(xmltag,"author")) author=s;
  else if(!STRCMP(xmltag,"coder")) coder=s; //2 new fixed fields
  else if(!STRCMP(xmltag,"artist")) artist=s; //2 new fixed fields
  else if(!STRCMP(xmltag,"email")) email=s;
  else if(!STRCMP(xmltag,"homepage")) homepage=s;
  else if(!STRCMP(xmltag,"parentskin")) parentskin=s;
  else if(!STRCMP(xmltag,"screenshot")) screenshot=s;
}
