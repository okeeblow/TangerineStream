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
#ifndef _OBJECTDIR_H
#define _OBJECTDIR_H

#include "common.h"
#include "../studio/services/svc_objectdir.h"
#include "../bfc/string.h"
#include "../studio/runlevelcb.h"
#include "../bfc/depend.h"
#include "../common/multimap.h"

// in general, you derive from ObjectDirT<classname>

class COMEXP ObjectDir : public svc_objectDirI, public DependentI, public RunlevelCallbackI {
protected:
  // each object in a directory is 'held' by an ObjRec
  class ObjRec {
  public:
    ObjRec(void *obj, const char *par) : handle(INVALID_OBJECT_HANDLE), object(obj), param(par), hidden(0), selectable(TRUE), sortorder(0) {}

    ObjectHandle handle;
    void *object;
    String label, path, param;
    String displaygroup, icon;
    int hidden, selectable, sortorder;
  };
  virtual void *_createObject(const char *param, ObjectHandle handle)=0;
  virtual void _deleteObject(void *object)=0;

//---------
  ObjectDir(const char *config_prefix, const char *dirname);
  virtual ~ObjectDir();

public:
  virtual const char *getDirType();
  virtual int getNumObjects();
  virtual ObjectHandle enumObject(int n);
  virtual const char *getObjectLabel(ObjectHandle handle);
  virtual int setObjectLabel(ObjectHandle handle, const char *newlabel);

  // insert and get are implemented in ObjectDirT below

  void setObjectPath(ObjectHandle handle, const char *path);
  void setObjectDisplayGroup(ObjectHandle handle, const char *group);
  void setObjectIcon(ObjectHandle handle, const char *icon_name);
  void setObjectSelectable(ObjectHandle handle, int selectable);
  void setObjectSortOrder(ObjectHandle handle, int sortorder);

  void setDefaultObjectDisplayGroup(const char *group);

  virtual int tagObject(const char *tag, ObjectHandle handle, int exclusive=FALSE);
  virtual int untagObject(const char *tag, ObjectHandle handle);
  virtual ObjectHandle enumObjectByTag(const char *tag, int n);
  virtual int isTagged(const char *tag, ObjectHandle handle);

  virtual int removeObject(ObjectHandle handle);
  virtual void clearAll();
  virtual const char *getObjectPath(ObjectHandle handle);
  virtual const char *getObjectDisplayGroup(ObjectHandle handle);
  virtual const char *getObjectIcon(ObjectHandle handle);
  virtual int getObjectSelectable(ObjectHandle handle);
  virtual int getObjectSortOrder(ObjectHandle handle);

protected:
  // override as needed
  virtual void onPrerender(ObjectHandle handle, const RECT *r, BaseCanvas *c, int style) { }
  virtual void onPostrender(ObjectHandle handle, const RECT *r, BaseCanvas *c, int style) { }
  virtual int onAction(int action, RootWnd *from, const char *target, ObjectHandle handle) { return 0; }
  virtual int contextMenu(RootWnd *from, int x, int y, ObjectHandle handle) { return 0; }
  virtual void objdir_onObjectCreation(void *obj, ObjectHandle handle) { }
  virtual void *castObject(void *obj) { return obj; }

//////////////

  virtual ObjectHandle insertObject(const char *param, const char *label=NULL, const char *path=NULL);
  virtual void *getObject(ObjectHandle handle);

  // lets you add an object you have created yourself
  ObjectHandle _addObject(void *object, const char *label=NULL, const char *path=NULL, const char *param=NULL);

//  // from RunlevelCallbackI
//  virtual void runlevelcb_onAppRunning();
//  virtual void runlevelcb_onShutdown();

private:
  virtual Dependent *getDependencyPtr() { return this; }

  ObjRec *handleToSlot(ObjectHandle handle);
  void untagObjRec(ObjRec *rec);

  int id_gen;
  String config_prefix, dir_name;
  String def_group;
  PtrList<ObjRec> objlist;
  MultiMap<String, ObjRec> tagmap;
};

template <class T, class CASTTO=T>
class ObjectDirT : public ObjectDir {
protected:
  ObjectDirT(const char *_config_prefix, const char *_dirname)
    : ObjectDir(_config_prefix, _dirname) { }

public:
  // if object==NULL, then it's a defer-create object. in which case the param
  // will be passed to createObject() below, which you can override to
  // apply the param
  ObjectHandle addObject(T *object, const char *label=NULL, const char *path=NULL, const char *param=NULL) {
    return _addObject(object, label, path, param);
  }
protected:
  T *getTypedObject(ObjectHandle handle) {
    return static_cast<T*>(ObjectDir::getObject(handle));
  }

  // overridable
  virtual void objdir_onObjectCreation(T *obj, ObjectHandle handle) { }

private:
  virtual void *_createObject(const char *param, ObjectHandle handle) {
    return new T(param);
  }
  virtual void _deleteObject(void *object) {
    delete static_cast<T*>(object);
  }
  virtual void objdir_onObjectCreation(void *obj, ObjectHandle handle) {
    objdir_onObjectCreation(static_cast<T*>(obj), handle);
  }
  virtual void *castObject(void *obj) {
    return static_cast<CASTTO*>(static_cast<T*>(obj));
  }
};

#include "contextcmd.h"
class ContextCmdObjDir : public ContextCmdT<svc_objectDir> {
public:
  ContextCmdObjDir(const char *objdirname, const char *menu_path=NULL) : objdir(objdirname), ContextCmdT<svc_objectDir>(menu_path) { }

protected:
  virtual int testItem(DragItem *item, const char *_menu_path) {
    if (!ContextCmd::testItem(item, _menu_path)) return 0;
    svc_objectDir *od = static_cast<svc_objectDir*>(item->getDatum(0));
    if (od == NULL || !objdir.isequal(od->getDirType())) return 0;
    return 1;
  }

private:
  String objdir;
};

#endif
