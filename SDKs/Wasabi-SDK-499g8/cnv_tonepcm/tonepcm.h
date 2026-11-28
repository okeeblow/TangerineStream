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
#ifndef _TONEPCM_H
#define _TONEPCM_H

#include "../studio/services/svc_mediaconverter.h"
#include "../studio/services/servicei.h"
#include "../studio/corecb.h"
#include "../studio/wac.h"

class TonePcm : public svc_mediaConverterI {
public:
  TonePcm();
  virtual ~TonePcm();

  // service
  static const char *getServiceName() { return "Tone generator"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(name && !STRNICMP(name,"tone://",7)) return 1; // only accepts "tone://..." urls
    return 0;
  }
  virtual const char *getConverterTo() { return "PCM"; }

  virtual int getInfos(MediaInfo *infos);

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void) { return 0; }

  // callbacks
  virtual int corecb_onSeeked(int newpos) { return 0; } // do nothing on seek

private:
  char sample_buffer[4096];
  double angle;
  int gen_freq;
};
#endif
