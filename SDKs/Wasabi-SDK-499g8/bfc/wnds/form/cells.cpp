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
#include <precomp.h>

#include "cells.h"

#include <bfc/wnds/blankwnd.h>
#include <bfc/wnds/dropdownlist.h>
#include <bfc/wnds/textbar.h>
#include <common/editwnd.h>

ColorCell::ColorCell(ARGB32 color) {
  bw = new BlankWnd(color);
  setContent(bw);
}

TextCell::TextCell(const char *name) {
  textbar = new TextBar();
  setContent(textbar);
  setText(name);
}

void TextCell::setText(const char *txt) {
  textbar->setName(txt);
}

int TextCell::setTextSize(int newsize) {
  return textbar->setTextSize(newsize);
}

void TextCell::setAlign(TextAlign align) { 
  textbar->setAlign(align); 
}

ButtonCell::ButtonCell() {
  buttonwnd = new ButtonWnd();
  setContent(buttonwnd);
}

void ButtonCell::setNotifyId(int id) {
  buttonwnd->setNotifyId(id);
}

int ButtonCell::setButtonText(char *text, int size) {
  return buttonwnd->setButtonText(text, size);
}

EditCell::EditCell() {
  editwnd = new EditWnd();
  setContent(editwnd);
}

void EditCell::setBuffer(char *buf, int len) {
  editwnd->setBuffer(buf, len);
}

#include <bfc/attribs/cfgitem.h>

CfgGroupCell::CfgGroupCell(CfgItem *_item, RootWnd *rw, const char *attrname)
  : item(_item) {
  item->onCfgGroupCreate(rw, attrname);
}

CfgGroupCell::~CfgGroupCell() {
  item->onCfgGroupDelete(getRootWnd());
  WASABI_API_SKIN->group_destroy(getRootWnd());
}

#include <common/guiobjwnd.h>

GroupCell::GroupCell(const char *_groupid, const char *_userdata)
: groupid(_groupid), userdata(_userdata) {
  group = new GuiObjectWnd();
  group->setContent(groupid);
  setContent(group);
}

void GroupCell::onInit() {
  GuiObject *go = group->getContent();
  go = go->guiobject_findObject(groupid);
  if (!userdata.isempty()) {
ASSERT(go != NULL);
    if (go != NULL)
      go->guiobject_setXmlParam("userdata", userdata);
  }
  int w = -1;//go->guiobject_getAutoWidth(); 
  int dw = ATOI(go->guiobject_getXmlParam("default_w"));
  if (dw != 0) w = dw;
  if (w > 0 && w != AUTOWH) setWidthFixed(w);

  int h = -1;//go->guiobject_getAutoHeight(); 
  int dh = ATOI(go->guiobject_getXmlParam("default_h"));
  if (dh != 0) h = dh;
  if (h > 0 && h != AUTOWH) setHeightFixed(h);
}


//-----------------------------------------------------------------------------


DropListCell::DropListCell(BOOL autosort) {
  droplist = new DropDownList();
//	droplist->setAutoSort(autosort);
  setContent(droplist);
}


int DropListCell::addItem(const char *text) {
 return droplist->addItem(text);
}

void DropListCell::delItem(int id) {
  droplist->delItem(id);
}

int DropListCell::findItem(const char *text) {
  return droplist->findItem(text);
}

int DropListCell::getNumItems() {
  return droplist->getNumItems();
}

DDLEntry *DropListCell::enumItem(int i) {
  return droplist->enumItem(i);
}
