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

#include "nosound.h"

NoSound::NoSound() {
  last = Std::getTimeStampMS();
  elapsed = 0.;
  last_srate = 0;
}

int NoSound::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  stdtimevalms now = Std::getTimeStampMS();
  Chunk *chunk = chunk_list->getChunk("PCM");
  if (chunk == NULL) return 0;
  int sr = chunk->getInfo("srate");
  if (sr > 0 && last_srate != sr) last_srate = sr;
  int siz = chunk->getSize();
  if (siz <= 0) {
    if (infos->getData("audio_need_canwrite")) {
      stdtimevalms diff = now - last;
      int ns = (int)(diff * (double)last_srate);
      infos->setDataInt("audio_canwrite", ns, MediaInfo::INFO_NOSENDCB);
      last = now;
    }
    return 1;
  }

  int bps = chunk->getInfo("bps");  //bits per sample
  int nch = chunk->getInfo("nch");  //number of channels

  if (bps < 1 || nch < 1) return 1;

  siz /= bps / 8;
  siz /= nch;

  // eat the samples
  stdtimevalms then = last + (double)siz / (double)last_srate;
  for (;;) {
    if ((now = Std::getTimeStampMS()) >= then) break;
    Sleep(1);
  }
  last = then;

  return 1;
}
