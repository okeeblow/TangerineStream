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
#ifndef _XUIQUERYLINE_H
#define _XUIQUERYLINE_H

#include "../bfc/wnds/queryline.h"

#define SCRIPTQUERYLINE_PARENT QueryLine
class ScriptQueryLine : public SCRIPTQUERYLINE_PARENT {
public:
  ScriptQueryLine();
  virtual ~ScriptQueryLine();

  //virtual int onInit();

  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);
  void setXuiQueryList(const char *v);

  //virtual int onDeferredCallback(int p1, int p2);

private:
  enum {
    QUERYLINE_SETQUERYLIST=1,
    QUERYLINE_SETQUERY,
    QUERYLINE_SETAUTO,
  };
  void ql_setQuery(const char *);
  void ensureConnected();
  int myxuihandle;
  String querylist_id;
};

extern char QueryLineXuiObjectStr[];
extern char QueryLineXuiSvcName[];
class QueryLineXuiSvc : public XuiObjectSvc<ScriptQueryLine, QueryLineXuiObjectStr, QueryLineXuiSvcName> {};

#endif
