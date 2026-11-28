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
#ifndef _CFGTREE_H
#define _CFGTREE_H

#include "../bfc/wnds/treewnd.h"

class CfgItem;	// see ../attribs/cfgitem.h
class CfgWnd;	// see ../attribs/cfgwnd.h

class CfgTreeItem : public TreeItemParam<CfgItem *> {
public:
  CfgTreeItem(CfgItem *item);

  virtual void onTreeAdd();
};

#include "../studio/metacb.h"

#define CFGTREEWND_PARENT TreeWnd
class CfgTreeWnd : public CFGTREEWND_PARENT, private MetaCallbackI {
public:
  CfgTreeWnd(CfgWnd *_cfgwnd);
  virtual ~CfgTreeWnd();

  virtual int onInit();
  virtual void onItemSelected(TreeItem *item);

  virtual void metacb_onPostScan();

private:
  CfgWnd *cfgwnd;
  String last_sel;
};
#endif
