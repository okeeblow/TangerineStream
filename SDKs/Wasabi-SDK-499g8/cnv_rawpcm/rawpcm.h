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
#ifndef _RAWPCM_H
#define _RAWPCM_H

#include "../studio/services/svc_mediaconverter.h"
#include "../studio/services/servicei.h"
#include "../studio/corecb.h"
#include "../studio/wac.h"
#include "../attribs/cfgitemi.h"
#include "../attribs/attrint.h"

class RawPcm : public svc_mediaConverterI {
public:
  RawPcm();
  virtual ~RawPcm();

  // service
  static const char *getServiceName() { return "RAW to PCM converter"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(name && !STRICMP(Std::extension(name),"raw")) return 1; // only accepts *.raw files
    return 0;
  }
  virtual const char *getConverterTo() { return "PCM"; }

  virtual int getInfos(MediaInfo *infos);

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void) { return 0; }

  // callbacks
  virtual int corecb_onSeeked(int newpos);

private:
  int filelen,needseek;
  char samplebuffer[4096];
};
#endif
