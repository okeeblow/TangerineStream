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
#include "infownd.h"
#include "info.h"

#include <bfc/wnds/form/cells.h>
#include <common/editwnd.h>
#include <common/checkwnd.h>
#include <common/droplist.h>
#include <common/script/c_script/c_button.h>
#include <common/script/c_script/c_dropdownlist.h>
#include <common/script/c_script/c_edit.h>
#include <common/script/scriptguid.h>
#include <common/metatags.h>
#include <common/filename.h>

InfoWnd *infownd=NULL;

class MyEditCell;
class MyEditWnd : public EditWnd {
public:
  MyEditWnd(MyEditCell *pparent) : parent(pparent) { }
  virtual void onEditUpdate();
private:
  MyEditCell *parent;
};
class MyEditCell: public FormCell {
public:
  MyEditCell(InfoWnd *pparent, const char *pname, const char *value, int maxsize) {
    parent=pparent;
    name=pname;
    editwnd = new MyEditWnd(this);
    setContent(editwnd);
    str.setSize(maxsize+1);
    STRCPY(str,value);
    editwnd->setBuffer(str.getMemory(),maxsize);
  }
  void onUpdate() { parent->updateInfo(name,str); }
private:
  MemBlock<char> str;
  MyEditWnd *editwnd;
  InfoWnd *parent;
  String name;
};
void MyEditWnd::onEditUpdate() {
  parent->onUpdate();
}

class CheckCell;
class MyCheckWnd: public CheckWnd {
public:
  MyCheckWnd(CheckCell *pparent) : parent(pparent) { }
  virtual int onClick();
private:
  CheckCell *parent;
};
class CheckCell : public FormCell {
public:
  CheckCell(InfoWnd *pparent, const char *pname, int checked) { 
    parent=pparent;
    name=pname;
    check=new MyCheckWnd(this); 
    setContent(check); 
    check->setChecked(checked); 
  }
  void onClick() {
    parent->updateInfo(name,StringPrintf("%i",check->isChecked()));
  }
private:
  CheckWnd *check;
  String name;
  InfoWnd *parent;
};
int MyCheckWnd::onClick() {
  parent->onClick();
  return 1;
}

class ComboCell;
class MyDropList : public DropList {
public:
  MyDropList(ComboCell *pparent) : parent(pparent) { }
  virtual void onItemSel(int pos);
private:
  ComboCell *parent;
};
class ComboCell : public FormCell {
public:
  ComboCell(InfoWnd *pparent, const char *pname, int pval, const char **pvalues) {
    parent=pparent;
    name=pname;
    list=new MyDropList(this);
    setContent(list);
    val=pval;
    values=pvalues;
  }
  virtual void init(FormWnd *parent) {
    FormCell::init(parent);
    const char **p=values;
    for(int i=0;p[i];i++) list->addItem(p[i]);
    list->setCurSel(val);
  }
  void onItemSel(int pos) {
    parent->updateInfo(name,StringPrintf("%i",pos));
  }
private:
  DropList *list;
  int val;
  const char **values;
  InfoWnd *parent;
  String name;
};
void MyDropList::onItemSel(int pos) {
  parent->onItemSel(pos);
}

int InfoWnd::onInit() {
  FormWnd::onInit();

  appcmds_addCmd("Save Changes", CMD_UPDATE);
  appcmds_addCmd("Reload", CMD_UNDO);
  //appcmds_addCmd("Advanced view", CMD_ADVANCED);

  getGuiObject()->guiobject_addAppCmds(this);

  infos=NULL;

  showLabel(TRUE);

  updateInfos();
  return 0;
}

InfoWnd::InfoWnd() {
  if(!infownd) infownd=this;
  getGuiObject()->guiobject_setDropTarget("info");
}

InfoWnd::~InfoWnd() {
  if(infownd==this) infownd=NULL;
  getGuiObject()->guiobject_removeAppCmds(this);
  delete(infos);
  buttonclickscallbacks.deleteAll();
  deleteAll();
}

void InfoWnd::appcmds_onCommand(int id, const RECT *buttonRect, int which_btn) {
  switch (id) {
    case CMD_UPDATE:
      commitChanges();
      break;
    case CMD_UNDO:
      // just refresh the view
      updateInfos();
      break;
  }
}

void InfoWnd::updateInfo(const char *name, const char *value) {
  if(infos) infos->SetMetaData(name,value);
}

#include "../common/profiler.h"

void InfoWnd::updateInfos() {
//  OutputDebugString("Setting content for infownd\n");
//  PR_ENTER("InfoWnd::updateInfos() - total");
  if(!current.isempty()) setName(current);
  else setName("No file");

  delete(infos);
  infos=NULL;
  buttonclickscallbacks.deleteAll();
  deleteAll();

  if(current.isempty()) {
    setContent(NULL);
    return;
  }

  infos=new Info();
  infos->Open(current);

  const char *content=infos->getXmlGroup();
  if(content) {
    PR_ENTER("InfoWnd::updateInfos() group shit");
//  PR_ENTER("InfoWnd::updateInfos() setContent");
    setContent(content);
//  PR_LEAVE();

    const char *n;
    for(int i=0;n=infos->enumName(i);i++) {
      int p1=0,p2=0;
      int attr=infos->enumAttributes(i,&p1,&p2);
      if(!(attr&MediaInfo::INFO_VISIBLE)) continue;
      const char *val=infos->enumValue(i);
      GuiObject *go=findObject(n);
      if(go) {
        if(attr&MediaInfo::INFO_EDIT_STRING) go->guiobject_setXmlParam("text",val);
        else if(attr&MediaInfo::INFO_EDIT_BOOL) {
          GuiObject *goct=go->guiobject_findObject("checkbox.toggle");
          if(!goct) continue;
          ScriptObject *so=goct->guiobject_getScriptObject();
          if(!so) continue;
          C_Button cb(so);
          cb.setActivated(ATOI(val));
        } else if(attr&MediaInfo::INFO_EDIT_COMBO) {
           XmlObject *goxml = static_cast<XmlObject*>(go->guiobject_getScriptObject()->vcpu_getInterface(xmlObjectGuid));
           if (goxml) {
             String xlist="";
             const char **list=(const char **)p1;
             for(int i=0;list[i];i++) {
               if (i > 0) xlist+=";";
               xlist+=list[i];
             }
             xlist+=";";
             goxml->setXmlParam("items", xlist);
             goxml->setXmlParam("select", list[ATOI(val)]);
           }
        } else if(attr&MediaInfo::INFO_TEXT) {
          go->guiobject_setXmlParam("default",val);
        }
      }
    }
    PR_LEAVE();

    // hardcoded id3 buttons
    {
      GuiObject *butGuiObj = getGuiObject()->guiobject_findObject("v1tov2");
      if (butGuiObj) buttonclickscallbacks.addItem(new ButtonClicksCallback(*butGuiObj, this, "v1tov2"));
      butGuiObj = getGuiObject()->guiobject_findObject("v2tov1");
      if (butGuiObj) buttonclickscallbacks.addItem(new ButtonClicksCallback(*butGuiObj, this, "v2tov1"));
    }
    
    return;
  }

//  PR_LEAVE();

  setContent(NULL);
  const char *n;
  for(int i=0;n=infos->enumName(i);i++) {
    const char *val=infos->enumValue(i);
    int p1=0,p2=0;
    int attr=infos->enumAttributes(i,&p1,&p2);
    if(!(attr&MediaInfo::INFO_VISIBLE)) continue;

    TextCell *t=new TextCell(n);
    t->setHeightFixed(16);
    t->setWidthFixed(100);
    newCell(t);

    if(attr&MediaInfo::INFO_EDIT_BOOL) {
      int check=0;
      if(val && val[0]=='1') check=1;
      CheckCell *v=new CheckCell(this,n,check);
      newCell(v);
    } else if((attr&MediaInfo::INFO_EDIT_STRING) && p1) {
      MyEditCell *v=new MyEditCell(this,n,val,p1);
      v->setHeightFixed(17);
      newCell(v);
    } else if((attr&MediaInfo::INFO_EDIT_COMBO) && p1) {
      ComboCell *v=new ComboCell(this,n,ATOI(val),(const char **)p1);
      v->setHeightFixed(24);
      newCell(v);
    } else {
      TextCell *v=new TextCell(val);
      if(attr&MediaInfo::INFO_DISPLAY_TIME) {
        if(!STRCMP(val,"-1")) v->setText("");
        else {
          int l=ATOI(val)/1000;
          v->setText(StringPrintf("%i:%02i",l/60,l%60));
        }
      }
      newCell(v);
    }
    
    nextRow();
  }

  recalcForm();
}

void InfoWnd::commitChanges() {
  if(infos) {
    if(infos->getXmlGroup()) {
      // fetch values
      const char *n;
      for(int i=0;n=infos->enumName(i);i++) {
        int p1=0,p2=0;
        int attr=infos->enumAttributes(i,&p1,&p2);
        if(!(attr&MediaInfo::INFO_VISIBLE)) continue;
        GuiObject *go=findObject(n);
        if(go) {
          if(attr&MediaInfo::INFO_EDIT_STRING) {
            ScriptObject *so=go->guiobject_getScriptObject();
            C_Edit e(so);
            const char *text=e.getText();
            if(text) infos->SetMetaData(n,text);
          }
          else if(attr&MediaInfo::INFO_EDIT_BOOL) {
            GuiObject *goct=go->guiobject_findObject("checkbox.toggle");
            if(!goct) continue;
            ScriptObject *so=goct->guiobject_getScriptObject();
            if(!so) continue;
            C_Button cb(so);
            StringPrintf res("%i",cb.getActivated());
            infos->SetMetaData(n,res);
          } else if(attr&MediaInfo::INFO_EDIT_COMBO) {
            C_DropDownList ddl(*go);
            const char *text = ddl.getItemSelected();
            if(text) infos->SetMetaData(n,text);
          }
        }
      }
    }
    getGuiObject()->guiobject_setStatusText("Updating...");
    if(infos->commitChanges()) {
      // success
      // reset db record
#define MT_NEEDINFO "_NeedInfo"
      int ni=1;
      api->metadb_setMetaData(current,MT_NEEDINFO,(const char *)&ni,sizeof(int),MDT_INT);
      getGuiObject()->guiobject_setStatusText("File updated.");
      //if(cfg_close) api->cmd_postMessage()
      if(cfg_close) api->skinwnd_destroy(this);
    } else getGuiObject()->guiobject_setStatusText("ERROR: Updating failed!");
  }
}


const char *InfoWnd::getWindowTypeName() { return "InfoWnd Window"; }
GUID InfoWnd::getWindowTypeGuid() { return infoeditwac->getGUID(); }
void InfoWnd::setIconBitmaps(ButtonWnd *button) {
  button->setBitmaps(infoeditwac->gethInstance(), IDB_TAB_NORMAL, NULL, IDB_TAB_HILITED, IDB_TAB_SELECTED);
}


int InfoWnd::dragEnter(RootWnd *sourceWnd) {
  FormWnd::dragEnter(sourceWnd);
  if (DI(sourceWnd).dragCheckData(DD_FILENAME) == -1) return 0;
  DI(sourceWnd).dragSetSticky(this, 0, 0, 32, 32);
  return 1;
}

int InfoWnd::dragDrop(RootWnd *sourceWnd, int x, int y) {
  int nitems,pos=-1;
  if ((pos=DI(sourceWnd).dragCheckData(DD_FILENAME,&nitems)) == -1) return 0;
  if(nitems && pos!=-1) {
    void *ptr = DI(sourceWnd).dragGetData(pos, 0);
    Filename *fn = static_cast<Filename *>(ptr);
    current=fn->getFilename();
    updateInfos();
  }
  return 0;
}

void InfoWnd::buttonClick(const char *id) {
  static const char *fields[]={
    "id3v1 track #","id3v2 track #",
    "id3v1 title","id3v2 title",
    "id3v1 artist","id3v2 artist",
    "id3v1 album","id3v2 album",
    "id3v1 year","id3v2 year",
    "id3v1 comment","id3v2 comment",
    NULL
  };
  if(STREQL(id,"v1tov2")) {
    for(int i=0;fields[i]!=NULL;i+=2) {
      const char *f1=fields[i];
      const char *f2=fields[i+1];
      GuiObject *go=findObject(f1);
      if(go) {
        GuiObject *go2=findObject(f2);
        if(go2) {
          ScriptObject *so=go->guiobject_getScriptObject();
          C_Edit e(so);
          const char *text=e.getText();
          go2->guiobject_setXmlParam("text",text);
        }
      }
    }
    GuiObject *go=findObject("id3v1 genre");
    if(go) {
      C_DropDownList ddl(*go);
      const char *text = ddl.getItemSelected();
      if(!text) text="";
      GuiObject *go2=findObject("id3v2 genre");
      if(go2) go2->guiobject_setXmlParam("text",text);
    }
    go=findObject("id3v2");
    if(go) {
      GuiObject *goct=go->guiobject_findObject("checkbox.toggle");
      if(goct) {
        ScriptObject *so=goct->guiobject_getScriptObject();
        if(so) {
          C_Button cb(so);
          cb.setActivated(1);
        }
      }
    }
    return;
  }
  if(STREQL(id,"v2tov1")) {
    for(int i=0;fields[i]!=NULL;i+=2) {
      const char *f1=fields[i];
      const char *f2=fields[i+1];
      GuiObject *go=findObject(f1);
      if(go) {
        GuiObject *go2=findObject(f2);
        if(go2) {
          ScriptObject *so=go2->guiobject_getScriptObject();
          C_Edit e(so);
          const char *text=e.getText();
          go->guiobject_setXmlParam("text",text);
        }
      }
    }
    GuiObject *go=findObject("id3v1");
    if(go) {
      GuiObject *goct=go->guiobject_findObject("checkbox.toggle");
      if(goct) {
        ScriptObject *so=goct->guiobject_getScriptObject();
        if(so) {
          C_Button cb(so);
          cb.setActivated(1);
        }
      }
    }
    go=findObject("id3v2 genre");
    if(go) {
      ScriptObject *so=go->guiobject_getScriptObject();
      C_Edit e(so);
      const char *text=e.getText();
      //TODO> update id3v1 genre
    }
    return;
  }
}
