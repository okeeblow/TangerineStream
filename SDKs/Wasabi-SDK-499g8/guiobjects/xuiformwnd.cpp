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

//<?#include "<class data="implementationheader"/>"
#include "XuiFormWnd.h"
//?>

#include "xuiformwnd.h"

#include <bfc/wnds/form/cells.h>
#include <bfc/parse/pathparse.h>

enum {
  CELLS,
};

XuiFormWnd::XuiFormWnd() {
  myxuihandle = newXuiHandle();

  addParam(myxuihandle, "cells", CELLS, XUI_ATTRIBUTE_REQUIRED);
//CUT: don't seem to require this anymore  setVirtual(TRUE);
}

int XuiFormWnd::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return XUIFORMWND_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case CELLS: {
      if (value == NULL) value = "";
      if (prev_cells.iscaseequal(value)) return TRUE;
      deleteAll();
DebugString("Cells=%s", value);
      PathParser pp(value, ";");
      for (int i = 0; i < pp.getNumStrings(); i++) {
        if (STREQL(pp.enumString(i), "/")) nextRow();
        else {
DebugString("substr %s", pp.enumString(i));
          PathParser split(pp.enumString(i), "=");
DebugString("split %s & %s", split.enumString(0), split.enumString(1));
          newCell(new GroupCell(split.enumString(0), split.enumString(1)));
        }
      }
      prev_cells = value;
      if (isPostOnInit()) onResize();
    }
    break;
    default: return 0;
  }

  return 1;
}

int XuiFormWnd::onInit() {
  XUIFORMWND_PARENT::onInit();

  // uhh

  return 1;
}

int XuiFormWnd::getContentsHeight() {
  return FormWnd::getContentsHeight();
}

void XuiFormWnd::onStuffHappened() {
  XUIFORMWND_PARENT::script_onStuffHappened();
}
