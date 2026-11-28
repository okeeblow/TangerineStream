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
#ifndef _XUIGROUPXFADE_H
#define _XUIGROUPXFADE_H

#include "../common/guiobjwnd.h"
#include "../bfc/wnds/blankwnd.h"

#define GROUPXFADE_PARENT GuiObjectWnd
class GroupXFade : public GROUPXFADE_PARENT {
public:
  static const char *xuiobject_getXmlTag() { return "GroupXFade"; }

  GroupXFade();
  virtual ~GroupXFade();
  
  virtual int onInit();
  
  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);
  virtual int onResize();

  enum {
    GROUPXFADE_SETGROUP,
    GROUPXFADE_SETSPEED,
  };

  void setNewGroup(const char *grp);

private:
  int myxuihandle;
  GuiObject *child[2];
  String id[2];
  int curchild;
  double speed;
};


class GroupXFadeXuiSvc : public XuiObjectSvc2<GroupXFade> {};

#endif
