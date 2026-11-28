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
#include "xuiextlistfeeder.h"
#include "../common/script/guiobject.h"
#include "../common/script/scriptobj.h"
#include "../common/script/scriptguid.h"
#include "../common/xmlobject.h"
#include "../studio/api.h"

// -----------------------------------------------------------------------
char MediaExtentionsFeederXuiObjectStr[] = "Parameters:MediaExtentions"; // This is the xml tag
char MediaExtentionsFeederXuiSvcName[] = "Parameters:MediaExtentions XuiObject Service"; 

void MediaExtentionsFeeder::actuator_onPerform(GuiObject *target) {
  XmlObject *xmltarget = static_cast<XmlObject *>(target->guiobject_getScriptObject()->vcpu_getInterface(xmlObjectGuid));
  if (!xmltarget) return;

  String list;
  const char *ext=api->core_getSupportedExtensions();
  const char *p=ext;
  int n=0;
  while(*p!=0 && *(p+1)!=0) {
    if(*p) {
      if (n>0) list += ";";
      list += p; n++;
    }
    while(*p!=0) p++;
    p++;
  }

  xmltarget->setXmlParam("items", list);
}
