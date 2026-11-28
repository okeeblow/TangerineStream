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
#include "precomp.h"

#include "objectdir.h"

ObjectDir::ObjectDir(const char *_config_prefix, const char *_dirname)
    : config_prefix(_config_prefix), dir_name(_dirname)
{
  id_gen = INVALID_OBJECT_HANDLE+1+9277;
  WASABI_API_SYSCB->syscb_registerCallback(this);
}

ObjectDir::~ObjectDir() {
  // delete the held objects
//FUCKO: ask if they want this, by default they do
  foreach(objlist)
    _deleteObject(objlist.getfor()->object);
  endfor
  objlist.deleteAll();
  WASABI_API_SYSCB->syscb_deregisterCallback(this);
}

const char *ObjectDir::getDirType() {
  return config_prefix;
}

int ObjectDir::getNumObjects() {
  return objlist.getNumItems();
}

ObjectHandle ObjectDir::enumObject(int n) {
  ObjRec *ret = objlist[n];
  if (ret == NULL) return INVALID_OBJECT_HANDLE;
  return ret->handle;
}

const char *ObjectDir::getObjectLabel(ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return NULL;
  return objrec->label;
}

int ObjectDir::setObjectLabel(ObjectHandle handle, const char *newlabel) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return 0;
  objrec->label = newlabel;
  dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_LABELCHANGE, handle);
  return 1;
}

void ObjectDir::setObjectSelectable(ObjectHandle handle, int val) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return;
  val = !!val;
  int prev = objrec->selectable;
  objrec->selectable = val;
  if (objrec->selectable != val)
    dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_SELECTABLECHANGE, handle);
}

void ObjectDir::setObjectSortOrder(ObjectHandle handle, int sortorder) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return;
  int prev = objrec->sortorder;
  objrec->sortorder = sortorder;
  if (prev != sortorder)
    dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_SORTORDERCHANGE, handle);
}

int ObjectDir::tagObject(const char *tag, ObjectHandle handle, int exclusive) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return 0;
  if (exclusive) {
    foreach(objlist)
      untagObject(tag, objlist.getfor()->handle);
    endfor
  } else untagObject(tag, handle);
  tagmap.multiAddItem(String(tag), objrec);
  dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_TAGCHANGE, handle);
  return 1;
}

int ObjectDir::untagObject(const char *tag, ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return 0;
  int r = tagmap.multiDelItem(String(tag), objrec);
  if (r) 
    dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_TAGCHANGE, handle);
  return r;
}

ObjectHandle ObjectDir::enumObjectByTag(const char *tag, int n) {
  ObjRec *rec=NULL;
  int r = tagmap.multiGetItem(String(tag), n, &rec);
  if (!r || rec == NULL) return INVALID_OBJECT_HANDLE;
  return rec->handle;
}

int ObjectDir::isTagged(const char *tag, ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return 0;
  return tagmap.multiHaveItem(String(tag), objrec);
}

ObjectHandle ObjectDir::insertObject(const char *param, const char *label, const char *path) {
  return _addObject(NULL, label, path, param);
}

ObjectHandle ObjectDir::_addObject(void *object, const char *label, const char *path, const char *param) {
  ObjRec *rec = new ObjRec(object, param);
  rec->handle = id_gen++;
  if (label == NULL)
    rec->label = param ? param : "[null]";
  else
    rec->label = label;
  rec->path = path;
  objlist.addItem(rec);
  dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_ADDED, rec->handle);
  return rec->handle;
}

void *ObjectDir::getObject(ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return NULL;
  if (objrec->object == NULL)
    objrec->object = _createObject(objrec->param, handle);
  objdir_onObjectCreation(objrec->object, handle);
  return castObject(objrec->object);
}

void ObjectDir::setObjectPath(ObjectHandle handle, const char *path) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return;
  int changed = !STREQLSAFE(objrec->path, path);
  objrec->path = path;
  if (changed)
    dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_PATHCHANGE, handle);
}

void ObjectDir::setObjectDisplayGroup(ObjectHandle handle, const char *group) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return;
  objrec->displaygroup = group;
}

void ObjectDir::setDefaultObjectDisplayGroup(const char *group) {
  def_group = group;
}

void ObjectDir::setObjectIcon(ObjectHandle handle, const char *icon_name) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return;
  objrec->icon = icon_name;
  dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_ICONCHANGE, handle);
}

const char *ObjectDir::getObjectIcon(ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return NULL;
  return objrec->icon;
}

int ObjectDir::getObjectSelectable(ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return FALSE;
  return objrec->selectable;
}

int ObjectDir::getObjectSortOrder(ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return 0;
  return objrec->sortorder;
}

const char *ObjectDir::getObjectPath(ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return NULL;
  return objrec->path;
}

const char *ObjectDir::getObjectDisplayGroup(ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return NULL;
  if (objrec->displaygroup.isempty()) return def_group;
  return objrec->displaygroup;
}

int ObjectDir::removeObject(ObjectHandle handle) {
  ObjRec *objrec = handleToSlot(handle);
  if (objrec == NULL) return 0;
  objlist.removeItem(objrec);
  _deleteObject(objrec->object);
  delete objrec;
  dependent_sendEvent(svc_objectDir::depend_getClassGuid(), svc_objectDir::Event_OBJECT_REMOVED, handle);
  untagObjRec(objrec);
  return 1;
}

void ObjectDir::clearAll() {
  while (getNumObjects()) removeObject(enumObject(0));
}

#if 0
void ObjectDir::runlevelcb_onAppRunning() {
  int npl = api->getIntPrivate(StringPrintf("%s/nobjects", config_prefix), 0);
  for (int i = 0; i < npl; i++) {
    char filename[WA_MAX_PATH]="";
    StringPrintf tag("%s/pl_%d", config_prefix, i);
    api->getStringPrivate(tag, filename, sizeof filename, "");
    char label[WA_MAX_PATH]="";
    tag.printf("%s/pl_%d_label", config_prefix, i);
    api->getStringPrivate(tag, label, sizeof label, "");
    insertObject(filename, label);
  }
}

void ObjectDir::runlevelcb_onShutdown() {
  StringPrintf npls("%s/objects", config_prefix);
  int prev_npl = api->getIntPrivate(StringPrintf("%s/objects", config_prefix), 0);
  foreach(objlist)
    objlist.getfor()->shutdown();
    StringPrintf tag("%s/obj_%d", config_prefix, foreach_index);
    api->setStringPrivate(tag, objlist.getfor()->filename);
    tag.printf("%s/obj_%d_label", config_prefix, foreach_index);
    api->setStringPrivate(tag, objlist.getfor()->label);
  endfor
  api->setIntPrivate(npls, objlist.getNumItems()+1);
}
#endif

void ObjectDir::untagObjRec(ObjRec *rec) {
  ASSERT(rec != NULL);
  tagmap.multiDelAllDataEntries(rec);
}

ObjectDir::ObjRec *ObjectDir::handleToSlot(ObjectHandle handle) {
  foreach(objlist)
    if (objlist.getfor()->handle == handle) return objlist.getfor();
  endfor
  return NULL;
}
