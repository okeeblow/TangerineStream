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
#ifndef __EDITBOX_H
#define __EDITBOX_H

#include "../bfc/wnds/embeddedxui.h"

#define  EDITBOX_PARENT EmbeddedXuiObject

// -----------------------------------------------------------------------
class EditBox : public EDITBOX_PARENT {
  public:

    virtual const char *embeddedxui_getContentId() { return "wasabi.edit"; }
    virtual const char *embeddedxui_getEmbeddedObjectId() { return "wasabi.edit.box"; }
};

// -----------------------------------------------------------------------
extern char EditBoxXuiObjectStr[];
extern char EditBoxXuiSvcName[];
class EditBoxXuiSvc : public XuiObjectSvc<EditBox, EditBoxXuiObjectStr, EditBoxXuiSvcName> {};

#endif
