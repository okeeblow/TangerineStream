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
#include "wasabicfg.h"
#include "addonreader.h"
#include "../studio/api.h"

// -----------------------------------------------------------------------
AddOnReader::AddOnReader(const char *filename){
  setFilename(filename);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/ScriptInfo",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/ScriptInfo/*",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/SkinModInfo",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/SkinModInfo/*",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/AddOnInfo",this);
  WASABI_API_XML->xmlreader_registerCallback("WinampAbstractionLayer/AddOnInfo/*",this);
  WASABI_API_XML->xmlreader_loadFile(filename,this);
  WASABI_API_XML->xmlreader_unregisterCallback(this);
}

// -----------------------------------------------------------------------
void AddOnReader::xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *s) {
  gotdata=1;
  if(!STRCMP(xmltag,"name")) fullname=s;
  else if(!STRCMP(xmltag,"version")) version=s;
  else if(!STRCMP(xmltag,"comment")) comment=s;
  else if(!STRCMP(xmltag,"author")) author=s;
  else if(!STRCMP(xmltag,"email")) email=s;
  else if(!STRCMP(xmltag,"homepage")) homepage=s;
}
