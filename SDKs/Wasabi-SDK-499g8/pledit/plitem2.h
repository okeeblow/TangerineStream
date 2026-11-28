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
#ifndef _PLITEM_H
#define _PLITEM_H

#include "../bfc/wnds/treewnd.h"
#include "../bfc/depview.h"
#include "playlist.h"
#include "svc_pldir.h"

class Editor;

enum PEI_type { PEI_PLAYLIST, PEI_FOLDER };

#define DD_PEITEM "DD_PEItem v1 (private)"

class NOVTABLE PEItem : public TreeItemParam<PEI_type> {
protected:
  PEItem(PEI_type type) : TreeItemParam<PEI_type>(type) {}

  virtual int onBeginDrag(char *suggestedTitle);

public:
  virtual void saveSettings(int &id)=0;
};

// a simple folder

#define FOLDERITEM_PARENT PEItem
class ObjDirFolderItem : public PEItem {
public:
  ObjDirFolderItem();

  virtual int onBeginLabelEdit() { return 0; }

  virtual int dragOver(RootWnd *sourceWnd);
  virtual int dragDrop(RootWnd *sourceWnd);

  virtual void saveSettings(int &id);
};

// this is a tree item that holds playlist handle
#define PLITEM_PARENT PEItem
class PlItem2 : public PEItem, public DependentViewerTPtr<Playlist> {
public:
  PlItem2(PlaylistHandle handle);
  virtual ~PlItem2();

  virtual void onSelect();
  virtual void onDeselect();
  virtual int onContextMenu(int x, int y);
  virtual int onChar(UINT key);

  virtual int onBeginLabelEdit();
  virtual int onEndLabelEdit(const char *newname);

  virtual int onLeftDoubleClick();

  virtual int customDraw(Canvas *canvas, const POINT &pt, int defaultTxtHeight, int indentation, const RECT &clientRect); 

  PlaylistHandle getPlaylistHandle();

  virtual int dragOver(RootWnd *sourceWnd);
  virtual int dragDrop(RootWnd *sourceWnd);

  virtual void saveSettings(int &id);

  Editor *getEditor();

// DependentViewerI
  virtual int viewer_onEvent(Playlist *item, int event, int param, void *ptr, int ptrlen);

  void resetLabel();	// sets label according to playlist name

private:
  PlaylistHandle handle;
  String plname;
  svc_plDir *pldir;
  int last_y;
};

#endif
