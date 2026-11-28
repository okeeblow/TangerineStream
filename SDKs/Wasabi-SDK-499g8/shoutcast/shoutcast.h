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
#ifndef _SHOUTCAST_H
#define _SHOUTCAST_H

#include "../studio/services/svc_mediaconverter.h"
//CUT #include "../studio/services/servicei.h"
//CUT #include "../studio/corecb.h"
//CUT #include "../studio/wac.h"
//CUT #include "../attribs/cfgitemi.h"
//CUT #include "../attribs/attrint.h"

class Shoutcast : public svc_mediaConverterI {
public:
  // service
  static const char *getServiceName() { return "Shoutcaster"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(chunktype && !STRICMP(chunktype,"PCM")) return 1;
    return 0;
  }
  virtual const char *getConverterTo() { return "PCM"; }

  virtual int getInfos(MediaInfo *infos) { return 0; }

  virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int corecb_onTitleChange(const char *title);
  
  virtual int sortPlacement(const char *otherconverter) { 
    return 1; // always try to be at the end
  }

private:
};


#endif
