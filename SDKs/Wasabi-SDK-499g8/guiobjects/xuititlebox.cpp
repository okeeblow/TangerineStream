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
#include "xuititlebox.h"

// -----------------------------------------------------------------------
char ScriptTitleBoxXuiObjectStr[] = "Wasabi:TitleBox"; // This is the xml tag
char ScriptTitleBoxXuiSvcName[] = "Wasabi:TitleBox XuiObject Service"; 

// -----------------------------------------------------------------------
ScriptTitleBox::ScriptTitleBox() : SCRIPTTITLEBOX_PARENT() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "title", SCRIPTTITLEBOX_TITLE, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "content", SCRIPTTITLEBOX_CONTENT, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "centered", SCRIPTTITLEBOX_CENTERED, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "suffix", SCRIPTTITLEBOX_SUFFIX, XUI_ATTRIBUTE_IMPLIED);
}

// -----------------------------------------------------------------------
ScriptTitleBox::~ScriptTitleBox() {
}

// -----------------------------------------------------------------------
int ScriptTitleBox::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTTITLEBOX_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  // Parcel the values out to the window object we multiply inherit from
  switch (xmlattributeid) {
    case  SCRIPTTITLEBOX_TITLE:
      setTitle(value);
    break;
    case  SCRIPTTITLEBOX_CONTENT:
      setChildGroup(value);
    break;
    case  SCRIPTTITLEBOX_CENTERED:
      setCentered(ATOI(value));
    break;
    case  SCRIPTTITLEBOX_SUFFIX:
      setSuffix(value);
    break;
    default:
      return 0;
  }
  return 1;
}
