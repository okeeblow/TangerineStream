#include "precomp.h"
#include "xuiquerylist.h"

// These are the strings used by the xuiobject service (see querylistwnd.h)

char QuerylistXuiObjectStr[] = "Querylist"; // This is the xml tag
char QuerylistXuiSvcName[] = "Querylist XuiObject Service"; // and this is the name of the service

// -----------------------------------------------------------------------

Querylist::Querylist() : scanner(NULL) {
  myxuihandle = newXuiHandle();
//  addParam(myxuihandle, "image", QUERYLISTWND_SETIMAGE, XUI_ATTRIBUTE_IMPLIED);
//  addParam("someotheroption", ID_SOMEOTHEROPTION, XUI_ATTRIBUTE_REQUIRED);
//  addParam("someotheroption", ID_SOMEOTHEROPTION, XUI_ATTRIBUTE_REQUIRED);
}

Querylist::~Querylist() {
}

int Querylist::onInit() {
  QUERYLIST_PARENT::onInit();

  addTreeItem(new TreeItem("blah"), NULL);

  return 1;
}

// XuiObject automatically calls this back for all parameters registered using addParam
// encountered in the xml source
int Querylist::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return QUERYLIST_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
//    case QUERYLISTWND_SETIMAGE:
//      setImage(value);
//      break;
/*    case ID_SOMEOTHEROPTION:
      setSomeOtherOption(value);
      break;
    case ID_SOMEOTHEROPTION:
      setSomeOtherOption(value);
      break;*/
    default:
      return 0;
  }
  return 1;
}

void Querylist::populate() {
#if 0
  if (joined_scanner != NULL) {
    scanner->getScanner()->unjoinScanner(joined_scanner->getScanner());
    joined_scanner->delRef();
    joined_scanner = NULL;
  }

  scheduleScannerDeletion(served_scanner);
  served_scanner = NULL;

  delete scanner;
  scanner = NULL;

  served_scanner = new SharedDbScanner(table);
  scanner = new dbScanner(table);

  if (!query.isempty()) {
    served_scanner->setQuery(query);
    scanner->setQuery(query);
  }

  if (cur_query_list) {
    joined_scanner = cur_query_list->getScanner();
    joined_scanner->addRef();
    scanner->getScanner()->joinScanner(joined_scanner->getScanner(), MT_PLAYSTRING);
  }

  scanner->first();
#endif
}
