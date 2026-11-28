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

#include "querydir.h"
#include <bfc/rootwnd.h>
#include <common/script/scriptguid.h>
#include <common/xmlobject.h>
#include <common/script/guiobject.h>
#include <common/script/scriptobj.h>
#include <core/corehandle.h>
#include <bfc/parse/pathparse.h>

QueryDir::QueryDir() : ObjectDirT<Query>("querydir", "Query Directory") {
  ObjectHandle h;
  h = insertObject("", "All Media");
  setObjectDisplayGroup(h, "library.display.allmusic");
  //FG> you can't query across tables unfortunately. i'll be adding that in the future but for now, play with the playstring
  //insertObject("playstring begins http and IsShoutcast = 0", "HTTP streamed music");
  h = insertObject("playstring begins http://", "HTTP Streams");
  setObjectDisplayGroup(h, "library.display.allmusic");
  h = insertObject("playstring begins cda://", "CD Tracks");
  setObjectDisplayGroup(h, "library.display.allmusic");

  // default group to show
  setDefaultObjectDisplayGroup("library.display.audio");
}

QueryDir::~QueryDir() {
}

int QueryDir::onAction(int action, RootWnd *from, const char *target, ObjectHandle handle) {
  switch (action) {
    case ODACTION_SELECTED: {
//CUT      FilenameNC qu(getObject(handle)->querytext);
//CUT      Filename *query = static_cast<Filename*>(getObject(handle));
      GuiObject *fromGuiObject = from->getGuiObject();
      if (fromGuiObject != NULL) {
        GuiObject *toGuiObject = fromGuiObject->guiobject_findObject(target);
        if (toGuiObject != NULL) {
          XmlObject *xmlobject = static_cast<XmlObject *>(toGuiObject->guiobject_getScriptObject()->vcpu_getInterface(xmlObjectGuid));
          if (xmlobject != NULL) {
            xmlobject->setXmlParam("order", "-1");
            xmlobject->setXmlParam("auto", "0");
            xmlobject->setXmlParam("query", getTypedObject(handle)->querytext.getValue());
          }
        }
      }
    }
    return 0;
  }

  return 0;
}

void QueryDir::runlevelcb_onStartup() {
  QUERYDIR_PARENT::runlevelcb_onStartup();
  ObjectHandle h;
  h = makeAutoQuery("Video");
  setObjectDisplayGroup(h, "library.display.video");
  h = makeAutoQuery("Audio");
  setObjectDisplayGroup(h, "library.display.audio");
}

ObjectHandle QueryDir::makeAutoQuery(const char *family) {
  CoreHandle ch;
  const char *extlist = ch.getSupportedExtensions(), *e = extlist;
  String s;
  for (;;) {
    const char *fam = ch.getExtensionFamily(e);
    PathParser pp(fam);
    if (pp.getNumStrings()) {
      for (int i = 0; i < pp.getNumStrings(); i++) {
        if (STRCASEEQL(pp.enumString(i), family)) {
          if (!s.isempty()) s.cat("|");
          s.cat(StringPrintf("(playstring ends .%s)", e));
          break;
        }
      }
    }
    for (; *e; e++);
    e++;
    if (*e == '\0') break;
  }
  return insertObject(s, StringPrintf("%s Files", family));
}
