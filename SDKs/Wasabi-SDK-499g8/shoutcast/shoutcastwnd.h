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
#ifndef __SHOUTCASTWND_H
#define __SHOUTCASTWND_H

#include "../bfc/virtualwnd.h"
#include "../bfc/bitmap.h"
#include "../bfc/canvas.h"
#include "../common/framewnd.h"
#include "../bfc/wnds/listwnd.h"
#include "../common/contwnd.h"
#include "../common/textbar.h"
#include "../common/editwndstring.h"
#include "../bfc/notifmsg.h"
#include "../common/checkwnd.h"
#include "../common/droplist.h"
#include "../bfc/appcmds.h"
#include "shoutcastwac.h"

class ServerListWnd : public ListWnd {
public:
  ServerListWnd();
  int onInit();

  void refresh();
  void updateList();
  int onRightClick(int itemnum);	// right-click on item
  void onItemSelection(int itemnum, int selected);
};
extern ServerListWnd *g_serverlistwnd;

/*class ServerWnd : public ContWnd {
public:
  ServerWnd();

  void update(int itemnum);
  int childNotify(RootWnd *child, int msg, int param1=0, int param2=0);
  int onInit();

private:
  TextBar nametext;
  TextBar urltext;
  TextBar genretext;
  TextBar aimtext;
  TextBar icqtext;
  TextBar irctext;

  TextBar pubtext;
  CheckWnd pubcheck;
  
  TextBar titleupdtext;
  CheckWnd titleupdcheck;

  TextBar hosttext;
  TextBar porttext;
  TextBar passwdtext;
  
  TextBar autocontext;
  CheckWnd autoconcheck;

  TextBar encodertext;
  DropList encoderdrop;

  EditWndString nameedit,genreedit,urledit, aimedit, icqedit, ircedit;
  EditWndString hostedit, portedit, passwdedit;
  
  int lastsel;
  int disable_notify;
};
*/

class ServerWnd : public LabelWnd {
public:
  ServerWnd();
  int onInit();
  void update(int itemnum);
};

class ListenerWnd : public LabelWnd {
public:
  ListenerWnd();

  int onInit();
  virtual void timerCallback(int id);
  virtual int onPaint(Canvas *c);
  int onResize();
  
private:
  int lshistory[1024];
};

/*class ShoutcastUpperWnd : public FrameWnd {
public:
	ShoutcastUpperWnd();

private:
  ServerListWnd *serverlist;
  ServerWnd *serverwnd;
};*/

#define SHOUTCASTWND_PARENT FrameWnd
class ShoutcastWnd : public SHOUTCASTWND_PARENT, public AppCmdsI {
public:
	ShoutcastWnd();
  virtual ~ShoutcastWnd();
  
  virtual int onInit();
  virtual void timerCallback(int id);

  virtual void appcmds_onCommand(int id, const RECT *buttonRect, int which_btn);
  
  virtual const char *getRootWndName() { return "Shoutcaster v" SC_VERSION; }
  
private:
  //ShoutcastUpperWnd *upper;
  ServerListWnd *upper;
  ListenerWnd *bottom;
};

extern ShoutcastWnd *shoutcastwnd;

#endif