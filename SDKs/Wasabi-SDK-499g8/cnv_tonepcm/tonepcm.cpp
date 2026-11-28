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
#include "tonepcm.h"
#include <math.h>

TonePcm::TonePcm() {
  gen_freq=2600;
  angle=0.0;
}

TonePcm::~TonePcm() {
}

int TonePcm::getInfos(MediaInfo *infos) {
  // length is -1 (infinite) by default

  const char *s=infos->getFilename()+7;
	int n=0;
	while (*s >= '0' && *s <= '9')
	{
		n*=10;
		n += *s++ -'0';
	}
	infos->setTitle(StringPrintf("%dhz Tone",n));
  infos->setInfo("44100hz 16bps mono");
  
  gen_freq=n;

  return 0;
}

int TonePcm::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
	double dangle=3.14159*2.0*(double)gen_freq/(double)44100.0;
	int l=sizeof(sample_buffer),x=l/2;
	short *s=(short *)sample_buffer;

  ChunkInfosI *ci=new ChunkInfosI();
  ci->addInfo("srate",44100);
  ci->addInfo("bps",16);
  ci->addInfo("nch",1);

	while (x--)
	{
		int i;
		double d = sin(angle)*32766.5;
		__asm {
			fld d
			fistp i
		}
		*s++=i;
		angle += dangle;
	}
  chunk_list->setChunk("PCM",sample_buffer,l,ci);

  return 1;
}
