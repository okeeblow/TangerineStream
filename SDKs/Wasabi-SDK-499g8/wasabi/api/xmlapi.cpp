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
#include "xmlapi.h"
#include "../../studio/xmlreader.h"

api_xml *xmlApi = NULL;

void XmlApi::xmlreader_registerCallback(const char *matchstr, XmlReaderCallback *callback) {
  if(callback && matchstr) XmlReader::registerCallback(matchstr, callback);
}

void XmlApi::xmlreader_unregisterCallback(XmlReaderCallback *callback) {
  if(callback) XmlReader::unregisterCallback(callback);
}

void XmlApi::xmlreader_loadFile(const char *filename, XmlReaderCallback *only_this_object) {
  if(filename) {
    String path;
    if(STRSTR(filename,"/") || STRSTR(filename,"\\")) {
      path=filename;
      char *p=(char *)path.getValue();
      *((char *)Std::scanstr_back(p,"/\\",p)+1)=0;
    } else path="";
    XmlReader::loadFile(Std::filename(filename), path, only_this_object);
  }
}

int XmlApi::xmlreader_startXmlLoad(const char *filename, XmlReaderCallback *only_this_object) {
  if(filename) return XmlReader::startXmlLoad(filename,only_this_object);
  return 0;
}

int XmlApi::xmlreader_loadChunk(int handle, int chunksize) {
  if(handle) return XmlReader::loadChunk(handle,chunksize);
  return 0;
}

void XmlApi::xmlreader_close(int handle) {
  if(handle) XmlReader::close(handle);
}
