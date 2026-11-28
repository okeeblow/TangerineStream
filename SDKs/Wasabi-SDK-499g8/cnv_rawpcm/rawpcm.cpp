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
#include "rawpcm.h"

extern _int nch;
extern _int samplerate;
extern _int bps;

RawPcm::RawPcm() {
  needseek=-1;
}

RawPcm::~RawPcm() {
}

int RawPcm::getInfos(MediaInfo *infos) {
  svc_fileReader *reader=infos->getReader();
  if(!reader) return 0;

  filelen=reader->getLength();
  infos->setLength(filelen/((bps==16?2:1)*nch*(samplerate/1000)));
  infos->setTitle(Std::filename(infos->getFilename()));
  infos->setInfo(StringPrintf("%ihz %ibps %s", samplerate, bps, nch==2?"stereo":"mono"));
  return 0;
}

int RawPcm::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  svc_fileReader *reader=infos->getReader();
  if(!reader) return 0;

  if(needseek!=-1) {
    reader->seek(needseek);
    needseek=-1;
  }

  int eof=0;
  int len=reader->read(samplebuffer,sizeof(samplebuffer));
  if(len!=sizeof(samplebuffer)) eof=1;

  ChunkInfosI *ci=new ChunkInfosI();
  ci->addInfo("srate",samplerate);
  ci->addInfo("bps",bps);
  ci->addInfo("nch",nch);

  chunk_list->setChunk("PCM",samplebuffer,len,ci);
  if(eof) return 0;
  return 1;
}

int RawPcm::corecb_onSeeked(int newpos) {
  needseek=(int)(newpos*(samplerate/1000))*nch*(bps==16?2:1);
  return 0;
}