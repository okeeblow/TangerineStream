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
#ifndef _WAVPCM_H
#define _WAVPCM_H

#include <windows.h>
#include <stdio.h> 
#include <mmreg.h>
#include <msacm.h>
#include "../studio/services/svc_mediaconverter.h"
#include "../studio/services/servicei.h"
#include "../studio/corecb.h"
#include "../studio/wac.h"

class WavPcm : public svc_mediaConverterI {
public:
  WavPcm() : m_opened(0),ateof(0) { }
  virtual ~WavPcm() { }

  // service
  static const char *getServiceName() { return "WA3 WAV to PCM converter"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(chunktype && !STRICMP(chunktype,"WAV")) return 1;
    if(name && !STRICMP(Std::extension((char *)name),"WAV")) return 1;
    return 0;
  }
  virtual const char *getConverterTo() { return "PCM"; }

  virtual int getInfos(MediaInfo *infos);

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  // callbacks
  virtual int corecb_onSeeked(int newpos);

private:
	int bytes_in_sbuf,bytes_in_rbuf;
  bool m_opened, ateof;
  WAVEFORMATEX fmt;
  char sbuf[1024*2*2];
	int ismono, is8bit;
  int use_acm;
  int lengthinms, lengthinbytes,posinbytes;
  int s_inputbuf;
  char *wav_inputbuf;
  ACMSTREAMHEADER ash;
  HMMIO wav_file;
  int start_in_wav_file, bps, blockalign, output_bsize;
  HACMSTREAM has;
  char m_buffer[4096];
};
#endif
