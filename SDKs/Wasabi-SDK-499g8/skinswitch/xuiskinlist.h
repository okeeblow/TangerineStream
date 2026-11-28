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
#ifndef __SKINLIST_H
#define __SKINLIST_H

#include "../bfc/wnds/treewnd.h"
#include "../bfc/ptrlist.h"
#include "../studio/xmlreader.h"

#define SKINLIST_PARENT TreeWnd

class cSkinInfo {
public:
  cSkinInfo(const char *_name, const char *_data) : name(_name), data(_data) { }

  String getData() {return data;}
  String getName() {return name;}

private:
  String data;
  String name;
};

// -----------------------------------------------------------------------
class SkinItem {
public:
  SkinItem(const char *pname, const char *pfilename) : name(pname),fullname(pname),filename(pfilename) { };
  const char *getName() { return name; }
  const char *getParentSkin() { return parentskin; }

  const char *getFullName() { return fullname; }
  const char *getVersion() { return version; }
  const char *getComment() { return comment; }
  const char *getAuthor() { return author; }
  const char *getCoder() { return coder; }
  const char *getArtist() { return artist; }
  const char *getEmail() { return email; }
  const char *getHomepage() { return homepage; }
  const char *getScreenshot() { return screenshot; }
  const char *getFilename() { return filename; }
protected:
  String name;
  String walversion;
  String parentskin;
  
  String fullname;
  String version;
  String comment;
  String author;
  String coder;
  String artist;
  String email;
  String homepage;
  String screenshot;
  String filename;
};

class SwitchTreeItem : public TreeItemParam<SkinItem *> {
public:
  SwitchTreeItem(SkinItem *p) : TreeItemParam<SkinItem *>(p) {
    setLabel(p->getName());
  }
};


// -----------------------------------------------------------------------
class SkinList : public SKINLIST_PARENT {
  
  public:

    SkinList();
    virtual ~SkinList();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setTarget(const char *elementname);
    void setInfoTarget(const char *elementname);
    void refreshSkinList();
    void onItemSelected(TreeItem *item);
    int onLeftButtonDblClk(int x, int y);
    virtual int onInit();
    virtual int onDeferredCallback(int p1, int p2);
    virtual int onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *src);
    virtual int wantHScroll() { return 0; }

  private:

    enum {
      SKINLIST_SETTARGET = 0,
      SKINLIST_SETINFOTARGET,
    };
  
    int myxuihandle;
    String target_id;
    String infotarget_id;
    PtrList<SwitchTreeItem> skinlist;
    int listInited;

    // todo: use a portable class
    WIN32_FIND_DATA fd;
    HANDLE f_handle;
    int ok;
    String curSkin;
    SwitchTreeItem *selitem;
};

// -----------------------------------------------------------------------
extern char SkinListXuiObjectStr[];
extern char SkinListXuiSvcName[];
class SkinListXuiSvc : public XuiObjectSvc<SkinList, SkinListXuiObjectStr, SkinListXuiSvcName> {};


// -----------------------------------------------------------------------
class SkinInfosXmlReader : public SkinItem, public XmlReaderCallbackI {
public:
  SkinInfosXmlReader(const char *skinname, const char *fullname);
  virtual void xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params);
  virtual void xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *s);
  virtual int xmlReaderDisplayErrors() { return 0; }
  virtual int xmlReaderIncludeFiles() { return 0; }
  virtual void load();
  private:
  String file;
  int loaded;
};


#endif
