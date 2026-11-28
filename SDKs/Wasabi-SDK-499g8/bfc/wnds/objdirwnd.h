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
#ifndef _OBJDIRWND_H
#define _OBJDIRWND_H

#include "treewnd.h"
#include "../../studio/services/svc_objectdir.h"
#include "../depview.h"

enum OBJ_type { OBJTYPE_OBJ, OBJTYPE_FOLDER };
class BaseObjItem : public TreeItemParam<OBJ_type> {
protected:
  BaseObjItem(OBJ_type type, const char *label) : TreeItemParam<OBJ_type>(type, label), sortorder(0) { }
public:
  int sortorder;
};

class ObjItem : public BaseObjItem {
public:
  ObjItem(ObjectHandle _handle, const char *label=NULL) : handle(_handle), BaseObjItem(OBJTYPE_OBJ, label), hittable(TRUE) {}
  
  ObjectHandle getObjectHandle() const { return handle; }

  virtual int onBeginLabelEdit() { return 0; } // allow editing
  virtual int isHitTestable() { return hittable; }

  int hittable;

private:
  ObjectHandle handle;
};

class ObjDirFolderItem : public BaseObjItem {
public:
  ObjDirFolderItem(const char *label) : BaseObjItem(OBJTYPE_FOLDER, label), dying(0) {
    setIcon(new SkinBitmap("player.button.thinger"));
  }
  virtual void onChildItemRemove(TreeItem *item) {
    if (getNumChildren() == 0) {
      dying = 1;
      getTree()->delItemDeferred(this);
    }
  }

  int dying;
};

#define OBJDIRWND_PARENT TreeWnd
class ObjDirWnd : public OBJDIRWND_PARENT, DependentViewerTPtr<svc_objectDir> {
public:
  ObjDirWnd();
  virtual ~ObjDirWnd();

  void setTargetDirName(const char *dirname);

  void setActionTarget(const char *targetname);

  void setDisplayTarget(const char *name);

  void setDefaultDisplay(const char *display);
  
  virtual int onInit();
  
  virtual int onContextMenu(int x, int y);
  virtual int onPreItemContextMenu(TreeItem *item, int x, int y);
  virtual int compareItem(TreeItem *p1, TreeItem *p2);
  
  virtual int viewer_onEvent(svc_objectDir *item, int event, int param2, void *ptr, int ptrlen);

protected:
  ObjItem *createObjItem(ObjectHandle handle);
  ObjItem *getObjItemForHandle(ObjectHandle handle);

  void folderify(ObjItem *item);

private:
  virtual void onItemSelected(TreeItem *item);
  virtual void onItemDeselected(TreeItem *item);

  svc_objectDir *objdir;
  String objectDirName, objectDirTarget, displayTarget, defaultDisplay;
  RootWnd *displaygroup;
  String displaygroupname;
};

template <class TREEITEMCLASS>
class ObjDirWndT : public ObjDirWnd {
public:
  virtual ObjItem *createObjItem(ObjectHandle handle) { return new TREEITEMCLASS(handle); }
};

#endif
