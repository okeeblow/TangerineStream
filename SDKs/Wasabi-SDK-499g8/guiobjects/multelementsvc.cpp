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
#include "multelementsvc.h"
#include "../studio/xmlreader.h"

#define XMLPROV_MULTIPLE_ELEMENTS "svc:multiple_elements"

// -----------------------------------------------------------------------
int MultipleElementsXmlProvider::testDesc(const char *desc) {
//CUT DebugString("compare %s", desc);
  if (STRCASEEQL(desc, XMLPROV_MULTIPLE_ELEMENTS)) return 1;
  return 0;
}

// -----------------------------------------------------------------------
const char *MultipleElementsXmlProvider::getXmlData(const char *desc, const char *incpath, XmlReaderParams *params) {

  if (!STRCASEEQL(desc, XMLPROV_MULTIPLE_ELEMENTS)) return NULL;

  ASSERT(params != NULL);

  const char *_filename = params->getItemValue("filename");
  const char *_id = params->getItemValue("id");
  const char *_numfiles = params->getItemValue("numfiles");
  const char *_filename_offset = params->getItemValue("filename_offset");
  int filename_offset = ATOI(_filename_offset);

  int numfiles = ATOI(_numfiles);
__asm int 3;

  xml = "";
  for (int i=0;i<numfiles;i++) {
    String filename(_filename);
    filename.replaceNumericField(i+filename_offset);
    String id(_id);
    id.replaceNumericField(i);
    xml += StringPrintf("<bitmap id=\"%s\" file=\"%s\" />\n", id.v(), filename.v());
  }

//CUT DebugString("Returning %s", xml.v());

  return xml;
}


