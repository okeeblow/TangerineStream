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
#ifndef _PLEDITSTATUS_H
#define _PLEDITSTATUS_H

#include "../common/guiobjwnd.h"
#include "../bfc/ptrlist.h"
#include <core/corecb.h>
#include "../bfc/depend.h"

class Playlist;

class Editor;

#define PLEDITSTATUS_PARENT GuiObjectWnd
class PlStatus : public PLEDITSTATUS_PARENT,
  public DependentViewerI, public CoreCallbackI  {
public:
  PlStatus();
  virtual ~PlStatus();

  virtual int onInit();

  virtual int setXmlParam(const char *param, const char *value);
  virtual int onPaint(Canvas *c);

  void setBkColor(COLORREF c);
  void setColor(COLORREF c);
  void setAntialias(int a);
  void setFontsize(int fs);
  void setBold(int b);
  void setFont(const char *f);
  void setOffsetX(int x);
  void setOffsetY(int y);

//CUT  virtual int viewer_onItemDeleted(Dependent *item);
//CUT  virtual int viewer_onEvent(Dependent *item, const GUID *classguid, int event, int param, void *ptr, int ptrlen);

  static GUID getWindowTypeGuid();
  static const char *getWindowTypeName() { return "Playlist Editor Status Window"; }

  virtual int corecb_onStatusMsg(const char *text);
  virtual int corecb_onTitleChange(const char *title);
  virtual void timerCallback(int id);
//CUT  static void onCreateEditor(Editor *e);

private:
  void setText(Playlist *pl);
//CUT  void initViewer(Editor *e);

  String font;

  int fgcolor_set;
  COLORREF fgcolor;
  int bgcolor_set;
  COLORREF bkcolor;

  int fontsize;
  int antialias;
  int bold;
  int offsetx, offsety;
  String title, len;
//CUT  int editor_inited;
//CUT  Editor *le_editor;

  Playlist *prevpl;

//XuiObject
  int myxuihandle;
};

extern char PlStatusXuiObjectStr[];
extern char PlStatusXuiObjectName[];
class PlStatusXuiSvc : public XuiObjectSvc<PlStatus, PlStatusXuiObjectStr, PlStatusXuiObjectName> {};

#endif
