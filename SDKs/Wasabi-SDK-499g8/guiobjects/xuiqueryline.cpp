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

#include "xuiqueryline.h"
#include "xuiquerylist.h"

#define CB_SETQUERYLIST 0x1978

char QueryLineXuiObjectStr[] = "QueryLine"; // This is the xml tag
char QueryLineXuiSvcName[] = "QueryLine XuiObject Service";

ScriptQueryLine::ScriptQueryLine() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "querylist", QUERYLINE_SETQUERYLIST, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "query", QUERYLINE_SETQUERY, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "auto", QUERYLINE_SETAUTO, XUI_ATTRIBUTE_IMPLIED);
}

ScriptQueryLine::~ScriptQueryLine() { }

/*int ScriptQueryLine::onInit() {
  SCRIPTQUERYLINE_PARENT::onInit();
  if (!querylist_id.isempty())
    postDeferredCallback(CB_SETQUERYLIST, 0, 500);
  return 1;
}*/

int ScriptQueryLine::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SCRIPTQUERYLINE_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case QUERYLINE_SETQUERYLIST:
      setXuiQueryList(value);
    break;
    case QUERYLINE_SETQUERY:
      ql_setQuery(value);
    break;
    case QUERYLINE_SETAUTO:
      setAuto(ATOI(value));
    break;
    default:
      return 0;
  }
  return 1;
}

void ScriptQueryLine::ql_setQuery(const char *q) {
  ensureConnected();
  setQuery(q);
}

void ScriptQueryLine::ensureConnected() {
  RootWnd *o = findWindow(querylist_id);
  if (!o) return;
  ScriptQueryList *querylist = static_cast<ScriptQueryList *>(o->getInterface(queryListGuid));
  if (!querylist) return;
  sqs_setMultiQueryServer(querylist);
}

void ScriptQueryLine::setXuiQueryList(const char *v) {
  querylist_id = v;
}

/*int ScriptQueryLine::onDeferredCallback(int p1, int p2) {
  switch (p1) {
    case CB_SETQUERYLIST:
      break;
    default:
      return SCRIPTQUERYLINE_PARENT::onDeferredCallback(p1, p2);
  }
  return 1;
}
*/