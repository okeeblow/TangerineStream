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

#include "switchwnd.h"

#include "../studio/api.h"

SwitchTreeWnd::SwitchTreeWnd(SkinFrameWnd *_skinframewnd) {
  skinframewnd = _skinframewnd;
  setName("Skins");
  showLabel(TRUE);
  setAutoCollapse(0);
}

void SwitchTreeWnd::onItemSelected(TreeItem *item) {
  SkinItem *skinitem = (static_cast<SwitchTreeItem *>(item))->getParam();
  skinframewnd->setSkinItem(skinitem);
}

int SwitchTreeWnd::onLeftButtonDblClk(int x, int y) {
  TreeItem *item=getCurItem();
  if(!item) return 0;
  SkinItem *skinitem = (static_cast<SwitchTreeItem *>(item))->getParam();
  api->skin_switchSkin((char *)skinitem->getName());
  return 1;
}


SkinFrameWnd::SkinFrameWnd() {
  child1 = new SkinShotWnd();
  child2 = new SkinCommentWnd();
  setChildren(child1, child2);
  setDividerType(DIVIDER_HORIZONTAL);
  setResizeable(TRUE);
  setDividerPos(SDP_FROMBOTTOM, 135);
}

void SkinFrameWnd::setSkinItem(SkinItem *item) {
  child1->setSkinItem(item);
  child2->setSkinItem(item);
}

SwitchWnd::SwitchWnd() {
  skinframewnd = new SkinFrameWnd();
  itemlist = new SwitchTreeWnd(skinframewnd);
  setChildren(itemlist, skinframewnd);
  setDividerType(DIVIDER_VERTICAL);
  setResizeable(TRUE);
  setDividerPos(SDP_FROMLEFT, 64);
  listInited=0;
}

SwitchWnd::~SwitchWnd() {
  // FrameWnd code will delete the child windows
  itemlist->deleteAllItems();
}

int SwitchWnd::onInit() {
  SWITCHWND_PARENT::onInit();
  itemlist->setSorted(FALSE);
  return 1;
}

void SwitchWnd::onNotifyActivate() {
  if(!listInited) refreshSkinList();
}

void SwitchWnd::refreshSkinList()
{
  itemlist->deleteAllItems();
  skinlist.removeAll();
//  itemlist->addTreeItem(new SwitchTreeItem(new SkinItem("Default")), NULL, FALSE, FALSE);
  const char *curSkin=api->getSkinName();
  SwitchTreeItem *selitem=NULL;

  WIN32_FIND_DATA fd;
  HANDLE f=FindFirstFile(StringPrintf("%s%c*", api->getSkinsPath(), DIRCHAR), &fd);
  int first=1,ok=1;
  while(f!=INVALID_HANDLE_VALUE && ok) {
    if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY || !STRICMP(Std::extension(fd.cFileName),"wal")
      || !STRICMP(Std::extension(fd.cFileName),"wsz") || !STRICMP(Std::extension(fd.cFileName),"zip")) {
      if(STRCMP(fd.cFileName,".")&&STRCMP(fd.cFileName,"..")) {
        // if its a file then strip extension
        if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
          char *p=(char *)Std::extension(fd.cFileName);
          if(p && *p) *(p-1)=0;
        }
//        if(STRICMP(fd.cFileName,"Default")) {
          SkinInfosXmlReader *r=new SkinInfosXmlReader(fd.cFileName);
          SwitchTreeItem *t=new SwitchTreeItem(r);
          skinlist.addItem(t);
          itemlist->addTreeItem(t, NULL, FALSE, FALSE);
          if(!STRICMP(fd.cFileName,curSkin)) selitem=t;
//        }
      }
    }
    ok=FindNextFile(f,&fd);
  }

  for(int i=0;i<skinlist.getNumItems();i++) {
    SkinItem *item=skinlist[i]->getParam();
    if(item->getParentSkin()) {
      // find its parent
      for(int j=0;j<skinlist.getNumItems();j++) {
        if(!STRICMP(item->getParentSkin(),skinlist[j]->getParam()->getName())) {
          // move item
          itemlist->moveTreeItem(skinlist[i], skinlist[j]);
          itemlist->expandItem(skinlist[j]);
        }
      }
    }
  }
  if(selitem) itemlist->selectItem(selitem);
  listInited=1;
}


SkinInfosXmlReader::SkinInfosXmlReader(char *skinname) : SkinItem(skinname) {
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/SkinInfo",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/SkinInfo/*",this);
  WASABI_API_XML->xmlreader_loadFile(StringPrintf("%s%s/skin.xml",api->getSkinsPath(),skinname),this);
  WASABI_API_XML->xmlreader_unregisterCallback(this);
}

void SkinInfosXmlReader::xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params) {
  if(!STRCMP(xmltag,"WinampAbstractionLayer")) walversion=params->getItemValue("version");
}

void SkinInfosXmlReader::xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *s) {
  if(!STRCMP(xmltag,"name")) fullname=s;
  else if(!STRCMP(xmltag,"version")) version=s;
  else if(!STRCMP(xmltag,"comment")) comment=s;
  else if(!STRCMP(xmltag,"author")) author=s;
  else if(!STRCMP(xmltag,"email")) email=s;
  else if(!STRCMP(xmltag,"homepage")) homepage=s;
  else if(!STRCMP(xmltag,"parentskin")) parentskin=s;
  else if(!STRCMP(xmltag,"screenshot")) screenshot=s;
}
