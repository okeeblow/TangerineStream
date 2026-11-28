#ifndef _QUERYLISTWND_H
#define _QUERYLISTWND_H

#include "../common/guiobjwnd.h"
#include "../bfc/wnds/treewnd.h"
#include "../bfc/wnds/autoquerylist.h"	// for QueryFilter

#define QUERYLIST_PARENT TreeWnd 
class Querylist : public QUERYLIST_PARENT {
public:

  Querylist();
  virtual ~Querylist();

  virtual int onInit();

  // XuiObject automatically calls this back for all parameters registered using addParam
  // encountered in the xml source
  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

protected:
  void populate();

private:
    // a list of IDs for our xml attributes, we use them in addParam() in the constructor
    enum {
      QUERYLISTWND_SETIMAGE        = 0,
  //    QUERYLISTWND_SOMEMOREOPTION,
  //    QUERYLISTWND_SOMEMOREOPTION,
    };
    int myxuihandle;

  dbScanner *scanner;
};


extern char QuerylistXuiObjectStr[];
extern char QuerylistXuiSvcName[];
class QuerylistXuiSvc : public XuiObjectSvc<Querylist, QuerylistXuiObjectStr, QuerylistXuiSvcName> {};

#endif
