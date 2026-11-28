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
#ifndef _SELWND_H
#define _SELWND_H

#include "../bfc/wnds/itemlistwnd.h"
#include "../bfc/depview.h"

class Playstring;
class Editor;
class svc_plDir;

#define SELWND_PARENT ItemListWnd
class SelWnd : public SELWND_PARENT, public DependentViewerTPtr<Editor> {
public:
  SelWnd();
  virtual ~SelWnd();

  virtual int onInit();

  static const char *getWindowTypeName() { return "Playlist Selection"; }

  virtual void onDoubleClick(int itemnum);	// double-click on an item
  virtual void onLeftClick(int itemnum);	// left-click

  virtual int viewer_onEvent(Editor *item, int event, int param2, void *ptr, int ptrlen);
  virtual int onDeferredCallback(int p1, int p2);

  virtual const char *convertlParam(LPARAM lParam);

private:
  int need_rescan;
  svc_plDir *pldir;
};

#endif
