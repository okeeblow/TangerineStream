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
#ifndef __PreferencesTree_H
#define __PreferencesTree_H

#include "../common/guiobjwnd.h"
#include "../bfc/autobitmap.h"
#include "../bfc/wnds/treewnd.h"
#include "preferences.h"

#define PREFERENCESTREE_PARENT TreeWnd 

// {3161708B-3CB8-4814-9363-AE65C1D2B6A3}
static const GUID prefTreeGuid = 
{ 0x3161708b, 0x3cb8, 0x4814, { 0x93, 0x63, 0xae, 0x65, 0xc1, 0xd2, 0xb6, 0xa3 } };

class PreferenceScreen : public TreeItem {
  public:

  PreferenceScreen(const char *grpid, const char *desc, GUID thisguid, GUID parentguid) :
    group_id(grpid),
    description(desc),
    this_guid(thisguid),
    parent_guid(parentguid),
    TreeItem(desc)
    { }
  virtual ~PreferenceScreen() { }

  const char *getGroupId() { return group_id; }
  const char *getDescription() { return description; }
  GUID getGuid() { return this_guid; }
  GUID getParentGuid() { return parent_guid; }

  private:
    String group_id;
    String description;
    GUID this_guid;
    GUID parent_guid;
};

// -----------------------------------------------------------------------

class PreferencesTree : public PREFERENCESTREE_PARENT {
  
  public:

    PreferencesTree();
    virtual ~PreferencesTree();

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setImage(const char *elementname);
    virtual int onInit();

    void onItemSelected(TreeItem *item);
    void openGroup(const char *groupid, int reselect=0);
    int onDeferredCallback(int p1, int p2);

  private:

    enum {
       PREFERENCESTREE_SETTARGET = 0,
    };
    int myxuihandle;

    void fillTree();
    PtrList<PreferenceScreen> screens;
    RootWnd *group;
    String curGroup;
    String target;
};


// -----------------------------------------------------------------------
extern char PreferencesTreeXuiObjectStr[];
extern char PreferencesTreeXuiSvcName[];
class PreferencesTreeXuiSvc : public XuiObjectSvc<PreferencesTree, PreferencesTreeXuiObjectStr, PreferencesTreeXuiSvcName> {};

#endif
