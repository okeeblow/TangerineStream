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
#include "xuiframe.h"
#include "../bfc/canvas.h"
#include "../studio/api.h"
#include "../common/script/scriptguid.h"

char ScriptFrameXuiObjectStr[] = "Wasabi:Frame"; // This is the xml tag
char ScriptFrameXuiSvcName[] = "Wasabi:ScriptFrame XuiObject Service"; 

ScriptFrame::ScriptFrame() {
  setVirtual(1);
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "orientation", SCRIPTFRAME_SETORIENTATION, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "left",        SCRIPTFRAME_SETLEFT,        XUI_ATTRIBUTE_IMPLIED); // top/bottom is alias for left/right
  addParam(myxuihandle, "top",         SCRIPTFRAME_SETLEFT,        XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "right",       SCRIPTFRAME_SETRIGHT,       XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "bottom",      SCRIPTFRAME_SETRIGHT,       XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "from",        SCRIPTFRAME_SETFROM,        XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "width",       SCRIPTFRAME_SETWIDTH,       XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "height",      SCRIPTFRAME_SETWIDTH,       XUI_ATTRIBUTE_IMPLIED); // height is an alias for width
  addParam(myxuihandle, "resizable",   SCRIPTFRAME_SETRESIZEABLE,  XUI_ATTRIBUTE_IMPLIED); 
  orientation = DIVIDER_VERTICAL;
  from = SDP_FROMLEFT;
  resizable = 1;
  width = 128;
  rootwndleft = rootwndright = NULL;
}

ScriptFrame::~ScriptFrame() {
  if (rootwndright) {
    WASABI_API_SKIN->group_destroy(rootwndright);
    rootwndright = NULL;
  }
  if (rootwndleft) {
    WASABI_API_SKIN->group_destroy(rootwndleft);
    rootwndleft = NULL;
  }
}

// XuiObject automatically calls this back for all parameters registered using addParam
// encountered in the xml source
int ScriptFrame::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {

  if (xuihandle != myxuihandle) 
    return SCRIPTFRAME_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case SCRIPTFRAME_SETORIENTATION: setOrientation(value); break;
    case SCRIPTFRAME_SETLEFT:        setLeft(value);        break;
    case SCRIPTFRAME_SETRIGHT:       setRight(value);       break;
    case SCRIPTFRAME_SETFROM:        setFrom(value);        break;
    case SCRIPTFRAME_SETWIDTH:       setWidth(value);       break;
    case SCRIPTFRAME_SETRESIZEABLE:  setResize(value);      break;
    default:
      return 0;
  }
  return 1;
}

int ScriptFrame::onInit() {

  if (!left.isempty()) rootwndleft = WASABI_API_SKIN->group_create(left);
  if (!right.isempty()) rootwndright = WASABI_API_SKIN->group_create(right);

/*  GuiObject *gl = rootwndleft ? static_cast<GuiObject *>(rootwndleft->getInterface(guiObjectGuid)) : NULL;
  GuiObject *gr = rootwndright ? static_cast<GuiObject *>(rootwndleft->getInterface(guiObjectGuid)) : NULL;

  if (gl) gl->guiobject_setParentGroup(getGuiObject()->guiobject_getParentGroup());
  if (gr) gr->guiobject_setParentGroup(getGuiObject()->guiobject_getParentGroup());*/

  if (rootwndleft) rootwndleft->setParent(this);
  if (rootwndright) rootwndright->setParent(this);
  
  setChildrenRootWnd(rootwndleft, rootwndright);
  setDividerType(static_cast<FrameWndDividerType>(orientation));
  setDividerPos(from, width);
  setResizeable(resizable);
  
  
  SCRIPTFRAME_PARENT::onInit();
  return 1;
}

void ScriptFrame::onResizeChildren(RECT leftr, RECT rightr) {
/*  if (rootwndleft) rootwndleft->invalidate();
  if (rootwndright) rootwndright->invalidate();*/
  invalidate();
}

void ScriptFrame::setOrientation(const char *orient) {
  if (STRCASEEQL(orient, "v") || STRCASEEQL(orient, "vertical"))
    orientation = DIVIDER_VERTICAL;
  if (STRCASEEQL(orient, "h") || STRCASEEQL(orient, "horizontal"))
    orientation = DIVIDER_HORIZONTAL;
}

void ScriptFrame::setLeft(const char *groupname) {
  left = groupname;
}

void ScriptFrame::setRight(const char *groupname) {
  right = groupname;
}

void ScriptFrame::setFrom(const char *f) {
  if (STRCASEEQL(f, "l") || STRCASEEQL(f, "left") || STRCASEEQL(f, "t") || STRCASEEQL(f, "top"))
    from = SDP_FROMLEFT;
  if (STRCASEEQL(f, "r") || STRCASEEQL(f, "right") || STRCASEEQL(f, "b") || STRCASEEQL(f, "bottom"))
    from = SDP_FROMRIGHT;
}

void ScriptFrame::setWidth(const char *w) {
  width = ATOI(w);
}

void ScriptFrame::setResize(const char *w) {
  resizable = ATOI(w);
  if (isInited())
    setResizeable(resizable);
}
