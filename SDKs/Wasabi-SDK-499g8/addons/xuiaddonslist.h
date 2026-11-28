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
#ifndef __SKINMODSLIST_H
#define __SKINMODSLIST_H

#include "../bfc/wnds/listwnd.h"

#define SKINMODSLIST_PARENT ListWnd

// -----------------------------------------------------------------------
class AddOnsList : public SKINMODSLIST_PARENT{
  
  public:

    AddOnsList();
    virtual ~AddOnsList();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    virtual int onInit();
    virtual void onDoubleClick(int itemnum);

    virtual int onAction(const char *action, const char *param, int x, int y, int p1, int p2, void *data, int datalen, RootWnd *source);

    int getModsType() { return modstype; }
    void setModsType(const char *val);
    virtual int wantHScroll() { return 0; }

  private:
    
    void addons_about();
    void addons_remove();
    void addons_add(int x, int y);

    int getSelectedMod();

    enum {
      SKINMODSLIST_SETMODSTYPE = 0,
    };

    int myxuihandle;
    int modstype;
};

// -----------------------------------------------------------------------
extern char AddOnsListXuiObjectStr[];
extern char AddOnsListXuiSvcName[];
class AddOnsListXuiSvc : public XuiObjectSvc<AddOnsList, AddOnsListXuiObjectStr, AddOnsListXuiSvcName> {};

#endif
