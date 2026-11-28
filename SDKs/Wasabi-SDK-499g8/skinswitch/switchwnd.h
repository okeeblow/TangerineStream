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
#ifndef _SWITCHWND_H
#define _SWITCHWND_H

#include "skinwnd.h"
#include "../bfc/wnds/framewnd.h"
#include "../bfc/wnds/treewnd.h"
#include "../bfc/ptrlist.h"
#include "../studio/xmlreader.h"

class SkinItem {
public:
  SkinItem(const char *name) : name(name),fullname(name) { };
  const char *getName() { return name; }
  const char *getParentSkin() { return parentskin; }


  const char *getFullName() { return fullname; }
  const char *getVersion() { return version; }
  const char *getComment() { return comment; }
  const char *getAuthor() { return author; }
  const char *getEmail() { return email; }
  const char *getHomepage() { return homepage; }
  const char *getScreenshot() { return screenshot; }
protected:
  String name;
  String walversion;
  String parentskin;
  
  String fullname;
  String version;
  String comment;
  String author;
  String email;
  String homepage;
  String screenshot;
};

class SwitchTreeItem : public TreeItemParam<SkinItem *> {
public:
  SwitchTreeItem(SkinItem *p) : TreeItemParam<SkinItem *>(p) {
    setLabel(p->getName());
  }
};

class SkinFrameWnd : public FrameWnd {
public:
  SkinFrameWnd();
  void setSkinItem(SkinItem *item);
  
private:
  SkinShotWnd *child1;
  SkinCommentWnd *child2;
};

class SwitchTreeWnd : public TreeWnd {
public:
  SwitchTreeWnd(SkinFrameWnd *_skinframewnd);

  virtual void onItemSelected(TreeItem *item);
  virtual int onLeftButtonDblClk(int x, int y);

private:
  SkinFrameWnd *skinframewnd;
};

class SkinInfosXmlReader : public SkinItem, public XmlReaderCallbackI {
public:
  SkinInfosXmlReader(char *skinname);
  virtual void xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params);
  virtual void xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *s);
  virtual int xmlReaderDisplayErrors() { return 0; }
};

#define SWITCHWND_PARENT FrameWnd
class SwitchWnd : public SWITCHWND_PARENT {
public:
  SwitchWnd();
  virtual ~SwitchWnd();

  virtual int onInit();
  virtual void onNotifyActivate();

  void refreshSkinList();

private:
  SwitchTreeWnd *itemlist;
  SkinFrameWnd *skinframewnd;
  
  int listInited;

  PtrList<SwitchTreeItem> skinlist;
};

#endif
