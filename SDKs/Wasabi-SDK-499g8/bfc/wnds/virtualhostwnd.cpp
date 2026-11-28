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
#include "virtualhostwnd.h"
#include "../canvas.h"
#include "../region.h"

VirtualHostWnd::VirtualHostWnd() {
  group = new GuiObjectWnd();
  fittoclient = 0;
  xoffset = 0;
  yoffset = 0;
  groupwidth = 0;
  groupheight = 0;
  scripts_enabled = 1;
}

VirtualHostWnd::~VirtualHostWnd() {
  delete group;
}

int VirtualHostWnd::onInit() {
  GuiObjectWnd::onInit();
  group->setVirtual(0);
  group->setStartHidden(1);
  group->setParent(this);
  group->init(this);
  group->deferedInvalidate();
  group->setCloaked(1); // magic!
  group->setVisible(1);
  return 1;
}

void VirtualHostWnd::virtualhostwnd_setContent(const char *_groupname) {
  group->setContent(_groupname);
  if (isPostOnInit())
    virtualhostwnd_onNewContent();
}

void VirtualHostWnd::virtualhostwnd_setContent(SkinItem *groupitem) {
  group->setContentBySkinItem(groupitem);
  if (isPostOnInit())
    virtualhostwnd_onNewContent();
}

void VirtualHostWnd::virtualhostwnd_onNewContent() {
}

#ifdef WASABI_COMPILE_SCRIPT
ScriptObject *VirtualHostWnd::virtualhostwnd_findScriptObject(const char *object_id) {
  return group->findScriptObject(object_id);
}
#endif

#ifdef WASABI_COMPILE_SKIN
GuiObject *VirtualHostWnd::virtualhostwnd_getContent() { 
  return group->getContent(); 
}

ScriptObject *VirtualHostWnd::virtualhostwnd_getContentScriptObject() { 
  return group->getContentScriptObject();
}

GuiObject *VirtualHostWnd::virtualhostwnd_findObject(const char *object_id) {
  return group->findObject(object_id);
}
#endif

RootWnd *VirtualHostWnd::virtualhostwnd_getContentRootWnd() { 
  return group->getContentRootWnd(); 
}

int VirtualHostWnd::onPaint(Canvas *c) {
  GuiObjectWnd::onPaint(c);

  virtualhostwnd_onPaintBackground(c);

  if (group == NULL) return 1;

  RECT wr;
  Canvas *cv = NULL;

  group->getNonClientRect(&wr);
  group->paint(NULL, NULL);

  cv = group->getFrameBuffer();

  if (cv != NULL) {
    BltCanvas *bltcanvas = static_cast<BltCanvas *>(cv); // HACK!
    bltcanvas->getSkinBitmap()->blitAlpha(c, xoffset, yoffset);
  }
  return 1;
}

void VirtualHostWnd::virtualhostwnd_setScriptsEnabled(int en) {
  group->abstract_setScriptsEnabled(en);
}

void VirtualHostWnd::virtualhostwnd_onPaintBackground(Canvas *c) {
  RECT r;
  getClientRect(&r);
  c->fillRect(&r, RGB(255,255,255));
}

int VirtualHostWnd::onResize() {
  GuiObjectWnd::onResize();
  if (group != NULL) {
    RECT r;
    getClientRect(&r);
    if (fittoclient) {
      xoffset = 0;
      yoffset = 0;
      groupwidth = r.right-r.left;
      groupheight = r.bottom-r.top;
    } else {
      groupwidth = group->getGuiObject()->guiobject_getAutoWidth();
      groupheight = group->getGuiObject()->guiobject_getAutoHeight();
      if (groupwidth  == AUTOWH) groupwidth  = 320;
      if (groupheight == AUTOWH) groupheight = 200;
      int cw = r.right-r.left;
      int ch = r.bottom-r.top;
      xoffset = (cw - groupwidth)/2;
      yoffset = (ch - groupheight)/2;
    }
    group->resize(xoffset+r.left, yoffset+r.top, groupwidth, groupheight);
  }
  return 1;
}

void VirtualHostWnd::virtualhostwnd_getContentRect(RECT *r) {
  ASSERT(r != NULL);
  getClientRect(r);
  r->left += xoffset;
  r->top += yoffset;
  r->right = r->left + groupwidth;
  r->bottom = r->top + groupheight;
}


void VirtualHostWnd::virtualhostwnd_fitToClient(int fit) {
  fittoclient = fit;
  if (isPostOnInit()) {
    onResize();
    invalidate();
  }
}

void VirtualHostWnd::onChildInvalidate(Region *r, RootWnd *who) {
  GuiObjectWnd::onChildInvalidate(r, who);
  Region *clone = r->clone();
  clone->offset(xoffset, yoffset);
  invalidateRgn(clone);
  r->disposeClone(clone);
}

int VirtualHostWnd::onLeftButtonDown(int x, int y) {
  // DO NOT CALL GuiObjectWnd::onLeftButtonDown(x, y);
  x -= xoffset;
  y -= yoffset;
  return group->wndProc(group->gethWnd(), WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
}

int VirtualHostWnd::onLeftButtonUp(int x, int y){
  // DO NOT CALL GuiObjectWnd::onLeftButtonUp(x, y);
  x -= xoffset;
  y -= yoffset;
  return group->wndProc(group->gethWnd(), WM_LBUTTONUP, 0, MAKELPARAM(x, y));
}

int VirtualHostWnd::onRightButtonDown(int x, int y){
  // DO NOT CALL GuiObjectWnd::onRightButtonDown(x, y);
  x -= xoffset;
  y -= yoffset;
  return group->wndProc(group->gethWnd(), WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(x, y));
}

int VirtualHostWnd::onRightButtonUp(int x, int y){
  // DO NOT CALL GuiObjectWnd::onRightButtonUp(x, y);
  x -= xoffset;
  y -= yoffset;
  return group->wndProc(group->gethWnd(), WM_RBUTTONUP, 0, MAKELPARAM(x, y));
}

int VirtualHostWnd::onLeftButtonDblClk(int x, int y){
  // DO NOT CALL GuiObjectWnd::onLeftButtonDblClk(x, y);
  x -= xoffset;
  y -= yoffset;
  return group->wndProc(group->gethWnd(), WM_RBUTTONDBLCLK, 0, MAKELPARAM(x, y));
}

int VirtualHostWnd::onRightButtonDblClk(int x, int y){
  // DO NOT CALL GuiObjectWnd::onRightButtonDblClk(x, y);
  x -= xoffset;
  y -= yoffset;
  return group->wndProc(group->gethWnd(), WM_LBUTTONDBLCLK, 0, MAKELPARAM(x, y));
}

int VirtualHostWnd::onMouseMove(int x, int y){
  // DO NOT CALL GuiObjectWnd::onMouseMove(x, y);
  x -= xoffset;
  y -= yoffset;
  return group->wndProc(group->gethWnd(), WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
}

