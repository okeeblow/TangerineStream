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

#include "svc_objectdir.h"
#include "../api.h"

#define CBCLASS svc_objectDirI
START_DISPATCH;
  CB(GETDEPENDENCYPTR, getDependencyPtr);
  CB(GETDIRTYPE, getDirType);
  CB(GETNUMOBJECTS, getNumObjects);
  CB(ENUMOBJECT, enumObject);
  CB(GETOBJECT, getObject);
  CB(GETOBJECTLABEL, getObjectLabel);
  CB(SETOBJECTLABEL, setObjectLabel);
  CB(INSERTOBJECT, insertObject);
  CB(REMOVEOBJECT, removeObject);
  VCB(CLEARALL, clearAll);
  CB(ONACTION, onAction);
  VCB(ONPRERENDER, onPrerender);
  VCB(ONPOSTRENDER, onPostrender);
  CB(GETOBJECTPATH, getObjectPath);
  CB(GETOBJECTDISPLAYGROUP, getObjectDisplayGroup);
  CB(GETOBJECTICON, getObjectIcon);
  CB(GETOBJECTSELECTABLE, getObjectSelectable);
  CB(GETOBJECTSORTORDER, getObjectSortOrder);
  CB(TAGOBJECT, tagObject);
  CB(UNTAGOBJECT, untagObject);
  CB(ENUMOBJECTBYTAG, enumObjectByTag);
  CB(ISTAGGED, isTagged);
  CB(CONTEXTMENU, contextMenu);
END_DISPATCH;
#undef CBCLASS
