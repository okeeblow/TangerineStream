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
#ifndef _PLDIRWND_H
#define _PLDIRWND_H

#include "../bfc/wnds/treewnd.h"
#include "svc_pldir.h"
#include "../bfc/depview.h"
#include "../studio/skincb.h"

class svc_plDir;
class PlItem2;
class WAComponentClient;
class Editor;

#define PLDIRWND_PARENT TreeWnd
class PlDirWnd : public PLDIRWND_PARENT, public DependentViewerI, public SkinCallbackI {
public:
  PlDirWnd();
  virtual ~PlDirWnd();

  static const char *getWindowTypeName() { return "Playlist Directory"; }

  virtual int onInit();
  virtual int onContextMenu(int x, int y);

  virtual int viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param2, void *ptr, int ptrlen);
  virtual int viewer_onItemDeleted(Dependent *item);

  static void PlDirWnd::registerServices(WAComponentClient *wac);

protected:
  PlItem2 *getPlItemForPlaylist(PlaylistHandle hand);

  virtual int skincb_onLoaded();

private:
  svc_plDir *pldir;
  PlaylistHandle selected_playlist;
  int select_first;
  Editor *editor;
};

#endif
