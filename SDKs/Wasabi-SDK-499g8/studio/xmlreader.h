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
#ifndef _XMLREADER_H
#define _XMLREADER_H

#include "../bfc/ptrlist.h"
#include "../bfc/string.h"
#include "../bfc/stack.h"
#include "../bfc/dispatch.h"
#include "xmlparams.h"

class svc_xmlProvider;

  typedef enum {
    TYPE_CLASS_CALLBACK=1,
    TYPE_STATIC_CALLBACK,
  } xmlreader_callbackType;

class xmlreader_cb_struct {
public:
  xmlreader_cb_struct(const char *m, xmlreader_callbackType t, XmlReaderCallback *cb) : matchstr(m), type(t), callback(cb) { }

  String matchstr;
  xmlreader_callbackType type;
  XmlReaderCallback * callback;
};


class XmlReaderCallback : public Dispatchable {
protected:
  XmlReaderCallback() {} // protect constructor

public:
  void xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params) {
    _voidcall(ONSTARTELEMENT, xmlpath, xmltag, params);
  }
  void xmlReaderOnEndElementCallback(const char *xmlpath, const char *xmltag) {
    _voidcall(ONENDELEMENT, xmlpath, xmltag);
  }
  void xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *str) {
    _voidcall(ONCHARDATA, xmlpath, xmltag, str);
  }
  int xmlReaderDisplayErrors() {
    return _call(DISPLAYERRORS, 1);
  }
  int xmlReaderIncludeFiles() {
    return _call(INCLUDEFILES, 1);
  }

  enum {
    ONSTARTELEMENT  = 100,
    ONENDELEMENT    = 200,
    ONCHARDATA      = 300,
    DISPLAYERRORS   = 1000,
    INCLUDEFILES    = 1100,
  };
};

class XmlReaderCallbackI : public XmlReaderCallback {
public:
  virtual void xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params) { }
  virtual void xmlReaderOnEndElementCallback(const char *xmlpath, const char *xmltag) { }
  virtual void xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *str) { }
  virtual int xmlReaderDisplayErrors() { return 1; }
  virtual int xmlReaderIncludeFiles() { return 1; }

#undef CBCLASS
#define CBCLASS XmlReaderCallbackI
START_DISPATCH;
      VCB(ONSTARTELEMENT, xmlReaderOnStartElementCallback);
      VCB(ONENDELEMENT, xmlReaderOnEndElementCallback);
      VCB(ONCHARDATA, xmlReaderOnCharacterDataCallback);
      CB(DISPLAYERRORS, xmlReaderDisplayErrors);
      CB(INCLUDEFILES, xmlReaderIncludeFiles);
END_DISPATCH;
#undef CBCLASS
};

class XmlFileHandle {
public:
  void *m_parser;
  FILE *m_file;
  String m_filename;
  XmlReaderCallback *m_callback;
};

class XmlReader {
public:
  // matchstr is a regexp string such as "WinampAbstractionLayer/Layer[a-z]"
  // or "Winamp*Layer/*/Layout"
  static void registerCallback(const char *matchstr, XmlReaderCallback *callback);
  static void registerCallback(const char *matchstr, void (*static_callback)(int start, const char *xmlpath, const char *xmltag, XmlReaderParams *params));

  static void unregisterCallback(void *callback);

  // if only_this_class param is specified, only this class will be called back
  // returns 1 on success, 0 on error
  static int loadFile(const char *filename, const char *incpath = NULL, void *only_this_callback = NULL, const char **params = NULL, int isinclude=0);
  static const char *getIncludePath();

  static int startXmlLoad(const char *filename, XmlReaderCallback *only_this_object = NULL);
  static int loadChunk(int handle, int chunksize);
  static void close(int handle);

  static int getNumCallbacks() { return callback_list.getNumItems(); }

private:
  static void parserCallbackOnStartElement(void *userData, const char *name, const char **parms);
  static void parserCallbackOnEndElement(void *userData, const char *nameC);
  static void parserCallbackOnCharacterData(void *userData, const char *str, int len);

  static const char *peekParam(const char *txt, const char **parms);
  static int doLoadFile(FILE *fp, svc_xmlProvider *svc, const char *filename, const char *incpath, void *only_this_callback, const char **params = NULL);

  static int parseBuffer(void *parser, const char *xml, int size, int final, XmlReaderCallback *only_this_callback, const char *filename);

  static PtrList<xmlreader_cb_struct> callback_list;
  static String currentpos;
  static String cont_char;
  static Stack<String *> include_stack;
};

#endif
