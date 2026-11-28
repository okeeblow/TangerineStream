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
#include "shoutcastwnd.h"
#include "shoutcastwac.h"
#include "server.h"

#include "../common/popup.h"
#include "../bfc/wnds/skinwnd.h"

ShoutcastWnd *shoutcastwnd;
ServerListWnd *g_serverlistwnd;
//ServerWnd *g_serverwnd;

extern _int graphtime;

enum {
  CMD_ADDNEWSERVER=100,
};
/*
ShoutcastUpperWnd::ShoutcastUpperWnd() {
  g_serverlistwnd = serverlist = new ServerListWnd();
  g_serverwnd = serverwnd = new ServerWnd();
  setChildren(serverlist, serverwnd);
  setDividerType(DIVIDER_VERTICAL);
  setResizeable(TRUE);
  setDividerPos(SDP_FROMRIGHT, 150);
}
*/
ShoutcastWnd::ShoutcastWnd() {
//  upper = new ShoutcastUpperWnd();
  g_serverlistwnd = upper = new ServerListWnd();
  bottom = new ListenerWnd();
  setChildren(upper, bottom);
  setDividerType(DIVIDER_HORIZONTAL);
  setResizeable(TRUE);
  setDividerPos(SDP_FROMBOTTOM, 90);
}

int ShoutcastWnd::onInit() {
  SHOUTCASTWND_PARENT::onInit();
  setTimer(200,500); // list update timer
  appcmds_addCmd("add new server", CMD_ADDNEWSERVER);
  getGuiObject()->guiobject_addAppCmds(this);
  return 1;
}

ShoutcastWnd::~ShoutcastWnd() {
  getGuiObject()->guiobject_removeAppCmds(this);
}

void ShoutcastWnd::appcmds_onCommand(int id, const RECT *buttonRect, int which_btn) {
  switch (id) {
    case CMD_ADDNEWSERVER:
      shoutcastwac->onCommand("add", 0, 0, NULL, 0);
    break;
  }
}

void ShoutcastWnd::timerCallback(int id) {
  if(id==200) g_serverlistwnd->updateList();
}

ServerListWnd::ServerListWnd() {
}

int ServerListWnd::onInit() {
  ListWnd::onInit();
/*  setName("Server list"); 
  showLabel(TRUE); */
  addColumn("Server name", 100);
  addColumn("# Listeners", 60);
  addColumn("Status", 200);

  refresh();
  //g_serverwnd->update(-1);  
  return 1;
}

void ServerListWnd::refresh() {
  deleteAllItems();

  for(int i=0;i<g_servers.getNumItems();i++) {
    Server *s=g_servers[i];
    addItem(s->name,(LPARAM)s);
  }

  updateList();
}

void ServerListWnd::updateList() {
  for(int i=0;i<g_servers.getNumItems();i++) {
    Server *s=g_servers[i];
    setItemLabel(i,(char *)s->name.getValue());
    StringPrintf nb("%i",s->nb_listeners);
    setSubItem(i,1,(char *)nb.getValue());
    int st=s->state;
    String state;
    switch(st) {
    case Server::STATE_NONE: state="Idle"; break;
    case Server::STATE_CONNECT: 
    case Server::STATE_CONNECTWAIT: 
      state="Connecting"; 
      break;
    case Server::STATE_CONNECTERROR:
      state.printf("Connect error! (%is)",s->getTimeout()/1000); 
      break;
    case Server::STATE_LOGIN:
    case Server::STATE_LOGINWAIT:
      state.printf("Login in (%is)",s->getTimeout()/1000); 
      break;
    case Server::STATE_LOGINERROR:
      state="Login error!";
      break;
    case Server::STATE_SENDHEADER:
      state="Sending headers";
      break;
    case Server::STATE_SEND: {
      int b=(int)(s->bytes_sent/1024);
      state.printf("Sending data (%iKbytes)",b);
      }
      break;
    case Server::STATE_SENDERROR:
      state.printf("Send error! (%is)",s->getTimeout()/1000);
      break;
    }
    setSubItem(i,2,(char *)state.getValue());
  }
}

int ServerListWnd::onRightClick(int itemnum) {
  Server *serv=g_servers[itemnum];
  PopupMenu menu(this);
  menu.addCommand("Connect", 100, FALSE, FALSE);
  menu.addCommand("Disconnect", 200, FALSE, FALSE);
  menu.addSeparator();
  menu.addCommand("Remove",300,FALSE,FALSE);
  menu.addSeparator();
  menu.addCommand("Properties",400,FALSE,FALSE);
  int ret = menu.popAtMouse();
  switch(ret) {
  case 100: {
    serv->close_connection();
    serv->state=Server::STATE_CONNECT;
    break;
  }
  case 200: {
    serv->state=Server::STATE_NONE;
    serv->close_connection();
    break;
  }
  case 300: { // remove
    //g_serverwnd->update(-1); // remove configs
    g_servers.removeByPos(itemnum);
    delete(serv);
    refresh();
    break;
  }
  case 400: { // properties
    // spawn edit dialog window
    //RootWnd *sw = api->skinwnd_createByGroupId("shoutcast.serverinfo");
    SkinWnd sw("shoutcast.serverinfo");
    sw.runModal(1);
    break;
  }
  }
  return 0;
}

void ServerListWnd::onItemSelection(int itemnum, int selected) {
  //if(selected) g_serverwnd->update(itemnum);
}



ListenerWnd::ListenerWnd() {
  setName("Listeners status graph"); 
  showLabel(TRUE);
  for(int i=0;i<sizeof(lshistory)/sizeof(int);i++) lshistory[i]=0;
}

int ListenerWnd::onInit() {
  LabelWnd::onInit();
  setTimer(100,graphtime*1000);
  return 1;
}

void ListenerWnd::timerCallback(int id) {
  if(id==100) {
    int m=sizeof(lshistory)/sizeof(int);
    m--;
    for(int i=m;i>0;i--) lshistory[i]=lshistory[i-1];
    int n=0;
    for(i=0;i<g_servers.getNumItems();i++) n+=g_servers[i]->nb_listeners;
    lshistory[0]=n;
    invalidate();
  }
}

int ListenerWnd::onPaint(Canvas *canvas) {
  LabelWnd::onPaint(canvas);

  RECT r;
  getClientRect(&r);
  
  int m=sizeof(lshistory)/sizeof(int);
  int maxnb=0;
  for(int i=0;i<m;i++) if(lshistory[i]>maxnb) maxnb=lshistory[i];
  if(maxnb<10) maxnb=10;

  {
    int sx=r.right-r.left;
    int sy=r.bottom-r.top;
    int p=sx-1;
    if(p>(m-1)) p=m-1;
    for(int i=0;i<sx && p;i++) {
      int x=r.left+i;
      int l=r.bottom-1-r.top;
      int y=r.bottom-1-(lshistory[p]*l)/maxnb;
      RECT r2={x,y,x+1,(r.bottom-1)};
      canvas->drawRect(&r2,1,RGB(200,200,200));
      RECT r3={x,r.top,x+1,y};
      canvas->drawRect(&r3,1,RGB(0,0,0));
      p--;
    }
  }
  return 0;
}

int ListenerWnd::onResize() {
  int r=LabelWnd::onResize();
  invalidate();
  return r;
}


/*
ServerWnd::ServerWnd() {
  setDeleteChilds(0);
  
  lastsel=-1;

  setName("Server config"); 
  showLabel(TRUE);

  addChild(&nametext,3,3,55,16);
  nametext.setName("Name");
  nametext.setTextSize(13);
  addChild(&nameedit,60,3,195,16);
  nameedit.setAutoEnter(1);

  addChild(&genretext,3,20,55,16);
  genretext.setName("Genre");
  genretext.setTextSize(13);
  addChild(&genreedit,60,20,195,16);
  genreedit.setAutoEnter(1);

  addChild(&urltext,3,37,55,16);
  urltext.setName("URL");
  urltext.setTextSize(13);
  addChild(&urledit,60,37,195,16);
  urledit.setAutoEnter(1);

  addChild(&aimtext,3,54,55,16);
  aimtext.setName("AIM");
  aimtext.setTextSize(13);
  addChild(&aimedit,60,54,70,16);
  aimedit.setAutoEnter(1);

  addChild(&icqtext,130,54,55,16);
  icqtext.setName("ICQ");
  icqtext.setTextSize(13);
  addChild(&icqedit,187,54,68,16);
  icqedit.setAutoEnter(1);

  addChild(&irctext,3,71,55,16);
  irctext.setName("IRC");
  irctext.setTextSize(13);
  addChild(&ircedit,60,71,70,16);
  ircedit.setAutoEnter(1);

  addChild(&pubcheck,3,92,12,12);
  addChild(&pubtext,18,90,200,16);
  pubtext.setName("Make this server public (Recommended)");
  pubtext.setTextSize(13);

  addChild(&titleupdcheck,3,110,12,12);
  addChild(&titleupdtext,18,108,110,16);
  titleupdtext.setName("Enable title updates");
  titleupdtext.setTextSize(13);

  addChild(&hosttext,3,140,55,16);
  hosttext.setName("Hostname");
  hosttext.setTextSize(13);
  addChild(&hostedit,60,140,195,16);
  hostedit.setAutoEnter(1);

  addChild(&porttext,3,157,55,16);
  porttext.setName("Port");
  porttext.setTextSize(13);
  addChild(&portedit,60,157,70,16);
  portedit.setAutoEnter(1);

  addChild(&passwdtext,130,157,55,16);
  passwdtext.setName("Password");
  passwdtext.setTextSize(13);
  addChild(&passwdedit,187,157,68,16);
  passwdedit.setAutoEnter(1);

  addChild(&autoconcheck,3,180,12,12);
  addChild(&autocontext,18,178,80,16);
  autocontext.setName("Auto-connect");
  autocontext.setTextSize(13);
  
  addChild(&encodertext,3,200,55,16);
  encodertext.setName("Encoder");
  encodertext.setTextSize(13);
  addChild(&encoderdrop,60,198,195,20);

//  Encoder::initConfig(0,this,3,224);
  disable_notify=0;
}

int ServerWnd::onInit() {
  ContWnd::onInit();
  update(-1);
  for(int i=0;;i++) {
    const char *c=Encoders::enumNames(i);
    if(!c) break;
    encoderdrop.addItem(c);
  }
  encoderdrop.setCurSel(0);
  return 1;
}

void ServerWnd::update(int itemnum) {
  // remove all encoder config screens
  for(int i=0;i<g_servers.getNumItems();i++) g_servers[i]->removeEncoderConfig();

  lastsel=itemnum;
  disable_notify=1;
  if(itemnum==-1) {
    nameedit.setBuffer("");
    genreedit.setBuffer("");
    urledit.setBuffer("");
    aimedit.setBuffer("");
    icqedit.setBuffer("");
    ircedit.setBuffer("");
    pubcheck.setChecked(0);
    titleupdcheck.setChecked(0);
    hostedit.setBuffer("");
    portedit.setBuffer("");
    passwdedit.setBuffer("");  
    autoconcheck.setChecked(0);
    enable(0);
    disable_notify=0;
    return;
  }
  enable(1);
  Server *s=g_servers[itemnum];
  nameedit.setBuffer(s->name,128);
  genreedit.setBuffer(s->genre,128);
  urledit.setBuffer(s->url,128);
  aimedit.setBuffer(s->aim,128);
  icqedit.setBuffer(s->icq,128);
  ircedit.setBuffer(s->irc,128);
  pubcheck.setChecked(s->pub);
  titleupdcheck.setChecked(s->title_upd);
  hostedit.setBuffer(s->host,128);
  portedit.setBuffer(StringPrintf("%i",s->port),16);
  passwdedit.setBuffer(s->passwd,40);  
  autoconcheck.setChecked(s->autoconnect);
  encoderdrop.setCurSel(s->encodernum);
  s->initEncoderConfig(this,3,224);
  s->updateEncoderConfig();
  disable_notify=0;
}

int ServerWnd::childNotify(RootWnd *child, int msg, int param1, int param2) {
  if(!disable_notify)
  if(msg==ChildNotify::EDITWND_DATA_MODIFIED || msg==ChildNotify::CHECKWND_CLICK || msg==ChildNotify::DROPLIST_SEL_CHANGED) {
    int sel=lastsel;//g_serverlistwnd->getFirstItemSelected();
    if(sel!=-1) {
      Server *s=g_servers[sel];
      s->name=nameedit.getBuffer();
      s->genre=genreedit.getBuffer();
      s->url=urledit.getBuffer();
      s->aim=aimedit.getBuffer();
      s->icq=icqedit.getBuffer();
      s->irc=ircedit.getBuffer();
      s->pub=pubcheck.isChecked();
      s->title_upd=titleupdcheck.isChecked();
      s->host=hostedit.getBuffer();
      s->port=ATOI(portedit.getBuffer());
      s->passwd=passwdedit.getBuffer();
      s->autoconnect=autoconcheck.isChecked();
      s->switchEncoder(encoderdrop.getCurSel());
      s->getEncoderConfig();
      g_serverlistwnd->updateList();
      if(s->state!=Server::STATE_NONE) {
        s->close_connection();
        s->state=Server::STATE_CONNECT;
      }
    }
  }
  return ContWnd::childNotify(child,msg,param1,param2);
}
*/

ServerWnd::ServerWnd() {
  setName("Server config"); 
  showLabel(TRUE);
}

int ServerWnd::onInit() {
  LabelWnd::onInit();
  setContent("shoutcast.serverinfo");
  update(-1);
  return 1;
}

void ServerWnd::update(int itemnum) {
  GuiObject *go=findObject("name");
  if(!go) return;
  go->guiobject_setXmlParam("text","blah");
}
