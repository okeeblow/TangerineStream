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

#include "attredit.h"

#include "../studio/api.h"
#include "../common/canvas.h"
#include "../common/metatags.h"

AttrEdit::AttrEdit(CfgItem *item, const char *name) {
  if (item != NULL && name != NULL)
    setAttr(item, name);
}

int AttrEdit::onPaint(Canvas *canvas) {
  PaintBltCanvas paintcanvas;
  if (canvas == NULL) {
    if (!paintcanvas.beginPaint(this)) return 0;
    canvas = &paintcanvas;
  }
  RECT r;
  getClientRect(&r);
  ATTREDIT_PARENT::onPaint(canvas);

  canvas->fillRect(&r, RGB(0, 0, 255));	// SKINME or something

  switch (cfgitem->getAttributeType(attrname)) {
    case AttributeType::INT: {
#if 0
        AttributeInt *aint = castAttr<AttributeInt>(attr);
        int val = aint->getValue();
        api->metadb_renderData(canvas, r, &val, MDT_INT);
#endif
    }
    break;
    case AttributeType::STRING: {
      MemBlock<char> tmp;
      int l = cfgitem->getDataLen(attrname)+1;
      tmp.setSize(l);
      cfgitem->getData(attrname, tmp.getMemory(), l);
      api->metadb_renderData(canvas, r, (void *)tmp.getMemory(), MDT_STRINGZ);
    }
    break;
  }

  return TRUE;
}

void AttrEdit::setAttr(CfgItem *item, const char *name) {
  viewer_delAllItems();
  ASSERT(item != NULL);
  ASSERT(name != NULL);
  cfgitem = item;
  attrname = name;
  viewer_addViewItem(cfgitem->getDependencyPtr());
}

const char *AttrEdit::getAttributeName() {
  return attrname;
}

void AttrEdit::dataChanged() {
  invalidate();
}

AttrEdit *AttrEdit::create(CfgItem *item, const char *name) {
  ASSERT(item != NULL);
  ASSERT(name != NULL);
  switch (item->getAttributeType(name)) {
    case AttributeType::INT:
      //return new AttrEditInt(item, name);
      return new AttrEditBool(item, name);

    case AttributeType::STRING:
      return new AttrEditString(item, name);
  }
  return NULL;
}

int AttrEdit::viewer_onItemAttrChange(Dependent *item, const char *attr) {
  if (!STRICMP(attr, attrname)) dataChanged();
  return 0;
}

AttrEditBool::AttrEditBool(CfgItem *item, const char *name) :
  AttrEdit(item, name) {
  checkwnd = new CheckWnd();
  checkwnd->setChecked(!!_intVal(item, name, 0));
}

AttrEditBool::~AttrEditBool() {
  delete checkwnd;
}

int AttrEditBool::onInit() {
  ATTREDITBOOL_PARENT::onInit();

  checkwnd->init(this);
  checkwnd->setParent(this);

  onResize();

  return 1;
}

int AttrEditBool::onResize() {
  ATTREDITBOOL_PARENT::onResize();

  RECT r = clientRect();

  r.right = r.left + (r.bottom - r.top);

  checkwnd->resizeToRect(&r);

  return 1;
}

int AttrEditBool::childNotify(RootWnd *child, int msg, int param1, int param2) {
  switch (msg) {
    case UMSG_CHECKWND_CLICK:
      _int_setValue(cfgitem, attrname, !!param1);
    return 1;
  }
  return ATTREDITBOOL_PARENT::childNotify(child, msg, param1, param2);
}

void AttrEditBool::dataChanged() {
  checkwnd->setChecked(_int_getValue(cfgitem, attrname));
  ATTREDITBOOL_PARENT::dataChanged();
}

AttrEdit_text::AttrEdit_text(CfgItem *item, const char *name) : AttrEdit(item, name) {
  editwnd = NULL;
}

AttrEdit_text::~AttrEdit_text() {
  delete editwnd;
}

int AttrEdit_text::onResize() {
  ATTREDIT_TEXT_PARENT::onResize();
  resizeToClient(editwnd);
  return 1;
}

void AttrEdit_text::onLeftPush(int x, int y) {
  editbuf.setSize(1024);
  int maxlen = convertToString(editbuf);
  if (maxlen <= 0) return;
  editwnd = new EditWnd(editbuf, maxlen);
  editwnd->init(this);
  editwnd->setParent(this);
  onResize();
}

int AttrEditInt::convertToString(MemBlock<char> &editbuf) {
  SPRINTF(editbuf, "%d", _int_getValue(cfgitem, attrname));
  return 10;
}

int AttrEditString::convertToString(MemBlock<char> &editbuf) {
  return editbuf.getSize()-1;
}
