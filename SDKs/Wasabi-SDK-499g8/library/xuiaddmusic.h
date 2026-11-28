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
#ifndef _XUIADDMUSIC_H
#define _XUIADDMUSIC_H

#include "../bfc/std.h"
#include "../common/guiobjwnd.h"
#include "../bfc/appcmds.h"
#include "../bfc/db/subqueryserver.h"

#define ADDMUSIC_PARENT GuiObjectWnd
class AddMusic : public ADDMUSIC_PARENT, AppCmdsI, public SubQueryServerI {
public:
  AddMusic();
  virtual ~AddMusic();

  virtual int onInit();

  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

  virtual void appcmds_onCommand(int id, const RECT *buttonRect, int which_btn);

protected:
  void setXuiQueryList(const char *v);
  virtual int onDeferredCallback(int p1, int p2);

private:
  int myxuihandle;
  String querylist_id;
};

extern char AddMusicXuiObjectStr[];
extern char AddMusicXuiSvcName[];
class AddMusicXuiSvc : public XuiObjectSvc<AddMusic, AddMusicXuiObjectStr, AddMusicXuiSvcName> {};

#endif
