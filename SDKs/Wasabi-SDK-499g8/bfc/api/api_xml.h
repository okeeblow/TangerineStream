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
#ifndef __API_XML_H
#define __API_XML_H

#include "../dispatch.h"

class XmlReaderCallback;

class api_xml : public Dispatchable {
  public:
    void xmlreader_registerCallback(const char *matchstr, XmlReaderCallback *callback);
    void xmlreader_unregisterCallback(XmlReaderCallback *callback);
    void xmlreader_loadFile(const char *filename, XmlReaderCallback *only_this_object = NULL);
    int xmlreader_startXmlLoad(const char *filename, XmlReaderCallback *only_this_object = NULL);
    int xmlreader_loadChunk(int handle, int chunksize);
    void xmlreader_close(int handle);

  enum {
    API_XML_REGISTERCALLBACK = 0,
    API_XML_UNREGISTERCALLBACK = 10,
    API_XML_LOADFILE = 20,
    API_XML_STARTXMLLOAD = 30,
    API_XML_LOADCHUNK = 40,
    API_XML_CLOSE = 50,
  };
};

inline void api_xml::xmlreader_registerCallback(const char *matchstr, XmlReaderCallback *callback) {
  _voidcall(API_XML_REGISTERCALLBACK, matchstr, callback);
}

inline void api_xml::xmlreader_unregisterCallback(XmlReaderCallback *callback) {
  _voidcall(API_XML_UNREGISTERCALLBACK, callback);
}

inline void api_xml::xmlreader_loadFile(const char *filename, XmlReaderCallback *only_this_object) {
  _voidcall(API_XML_LOADFILE, filename, only_this_object);
}

inline int api_xml::xmlreader_startXmlLoad(const char *filename, XmlReaderCallback *only_this_object) {
  return _call(API_XML_STARTXMLLOAD, (int)0, filename, only_this_object);
}

inline int api_xml::xmlreader_loadChunk(int handle, int chunksize) {
  return _call(API_XML_LOADCHUNK, (int)0, handle, chunksize);
}

inline void api_xml::xmlreader_close(int handle) {
  _voidcall(API_XML_CLOSE, handle);
}

class api_xmlI : public api_xml {
  public:
    virtual void xmlreader_registerCallback(const char *matchstr, XmlReaderCallback *callback)=0;
    virtual void xmlreader_unregisterCallback(XmlReaderCallback *callback)=0;
    virtual void xmlreader_loadFile(const char *filename, XmlReaderCallback *only_this_object = NULL)=0;
    virtual int xmlreader_startXmlLoad(const char *filename, XmlReaderCallback *only_this_object = NULL)=0;
    virtual int xmlreader_loadChunk(int handle, int chunksize)=0;
    virtual void xmlreader_close(int handle)=0;

  protected:
    RECVS_DISPATCH;
};

// {D327E1FB-21A8-4144-888E-3011E878F4A8}
static const GUID xmlApiServiceGuid = 
{ 0xd327e1fb, 0x21a8, 0x4144, { 0x88, 0x8e, 0x30, 0x11, 0xe8, 0x78, 0xf4, 0xa8 } };

extern api_xml *xmlApi;

#endif
