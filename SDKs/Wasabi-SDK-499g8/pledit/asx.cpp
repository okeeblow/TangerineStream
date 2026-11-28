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
#include "asx.h"

#include "../studio/api.h"
#include "../common/metatags.h"

#include "../bfc/xml/xmlconfig.h"
#include "../studio/xmlreader.h"

static const char * ASX_ROOT      = "Asx";
static const char * ASX_TITLE     = "Title";
static const char * ASX_ENTRY     = "Entry";
static const char * ASX_REF       = "Ref";
static const char * ASX_REF_HREF  = "Href";

static int inentry = 0;

static char curPlaystring[WA_MAX_PATH] = {0};

AsxLoader::AsxLoader() {
  inentry = 0;
}

int AsxLoader::readPlaylist(const char *filename) {
  WASABI_API_XML->xmlreader_registerCallback("ASX/*", this);
  WASABI_API_XML->xmlreader_loadFile(filename, this);
  WASABI_API_XML->xmlreader_unregisterCallback(this);

  return 1;
}

void AsxLoader::xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params) {
  if(!STRICMP(xmltag, ASX_ENTRY)) inentry = 1;
  if(!STRICMP(xmltag, ASX_REF) && (inentry == 1)) {
    STRNCPY(curPlaystring, params->getItemValue(ASX_REF_HREF), sizeof(curPlaystring)-1);
    addPlaylistEntry(curPlaystring);
  }
}

void AsxLoader::xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *str) {
  if(!STRICMP(xmltag, ASX_TITLE) && !inentry) {
      setName(str);
  }
}

void AsxLoader::xmlReaderOnEndElementCallback(const char *xmlpath, const char *xmltag) {
  if(!STRICMP(xmltag, ASX_ENTRY)) inentry = 0;
}