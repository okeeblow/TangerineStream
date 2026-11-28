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
#ifndef __XUIFORMWND_IMPL_H
#define __XUIFORMWND_IMPL_H

/*<?<autoheader/>*/
#include "XuiFormWndSO.h"
/*?>*/

#include <bfc/wnds/form/formwnd.h>

// -------------------------------------------------------
// Class XuiFormWnd (scriptable)
// -------------------------------------------------------
// {2D2D1376-BE0A-4CB9-BC0C-57E6E4C999F5}
static const GUID XuiFormWndGuid = 
{ 0x2d2d1376, 0xbe0a, 0x4cb9, { 0xbc, 0x0c, 0x57, 0xe6, 0xe4, 0xc9, 0x99, 0xf5 } };



class StackCell : public FormCell {
  public:
};

#define XUIFORMWND_PARENT XuiFormWndScriptObject

class XuiFormWnd : public XUIFORMWND_PARENT {
public:
  XuiFormWnd();

  static const char *xuiobject_getXmlTag() { return "Form"; }
  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

  virtual int onInit();

  SCRIPT virtual int getContentsHeight();
  SCRIPT EVENT virtual void onStuffHappened();

  addGroup(const char *groupid);

private:

  int myxuihandle;

  String prev_cells;
};

class XuiFormWndXuiSvc : public XuiObjectSvc2<XuiFormWnd> {};

#endif // __XUIFORMWND_IMPL_H
