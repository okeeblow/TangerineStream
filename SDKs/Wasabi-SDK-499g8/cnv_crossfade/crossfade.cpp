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
#include "crossfade.h"
#include <bfc/attribs/cfgitemi.h>
#include <bfc/attribs/attribs.h>

extern _bool crossfade_on_next;
extern _int crossfade_time;

Crossfade::Crossfade() {
  fade_pos=-1;
  last_srate=0;
  last_bps=0;
  last_nch=0;
  tmpbuf=NULL;
  tmpbufsize=-1;
  ci = NULL;
}

Crossfade::~Crossfade() {
  if(tmpbuf) FREE(tmpbuf);
  buf.close();
}

int Crossfade::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  char *s;
  int numsamples;

  // disable for http streams
  const char *filename=infos->getFilename();
  if(STRSTR(filename,"http://")==filename) return 1;

  // disable when video is present
  if(chunk_list->getChunk("VIDEO")) return 1;
  
  if(chunk_list->getChunk("NOXFADE")) return 1;

  Chunk *chunk=chunk_list->getChunk("PCM");
  if(chunk) {
    s=(char *)chunk->getData();
    if(!s) return 1;
    numsamples=chunk->getSize();
  } else {
    numsamples=0;
  }

  // no new samples, give 'em 4k of ours
  if (!numsamples) {
    int a=buf.bytesInBuf();
    if (a > 0)
    {
      if (a > 4096) a=4096;
      resizeTempBuffer(a);
      buf.getBytes(tmpbuf,a);
      ChunkInfosI *cinfos=new ChunkInfosI();
      cinfos->addInfo("nch",last_nch);
      cinfos->addInfo("bps",last_bps);
      cinfos->addInfo("srate",last_srate);
      chunk_list->setChunk("PCM",tmpbuf,a,cinfos);
      return 1;
    }
/*    chunk_list->setChunk("PCM",tmpbuf,0,cinfos);
    return 1;*/
    return 0;
  }

  int bps=chunk->getInfo("bps");
  int nch=chunk->getInfo("nch");
  int srate=chunk->getInfo("srate");
  if ((bps == 8 || bps == 16) && (nch == 1 || nch == 2) && srate) {

    // if they changed output params, reset buffer?
    if (last_srate!=srate || last_bps!=bps || last_nch!=nch || !buf.isOpen()) {
      fade_pos=-1;
      last_srate=srate;
      last_bps=bps;
      last_nch=nch;
      if(crossfade_time<1) crossfade_time=1; //FUCKO> do that in the attrib itself also 0 should just disable the xfader
      buf.open((last_bps>>3)*last_nch*last_srate*crossfade_time);
    }

    // we are in a fade
    if (fade_pos >= 0) {
      int fade_len=buf.getSize()-fade_pos;
      if (fade_len > numsamples) fade_len=numsamples;
      if (fade_len <= 0) {
        fade_pos=-1;
      } else {
        resizeTempBuffer(fade_len);
        MEMZERO(tmpbuf,fade_len);
        buf.getBytes(tmpbuf,fade_len);
        // fade it into the big one

        if (last_bps == 16 && last_nch == 1)
        {
          short int *a, *b;
          a=(short int *)s;
          b=(short int *)(tmpbuf);
          int x;
          int p=fade_pos;
          int l=fade_len/2;
          for (x = 0; x < l; x ++)
          {
            double d1;
            d1=(double)p/(double)buf.getSize();
            double d;
            d=*a * d1 + *b * (1.0-d1);
            int r=(int)d;
            if (r < -32768) r=-32768;
            if (r > 32767) r=32767;
            *a++ = (short) r; b++;
            p++;
          }
        }
        if (last_bps == 16 && last_nch == 2)
        { 
          short int *a, *b;
          a=(short int *)s;
          b=(short int *)(tmpbuf);
          int x;
          int p=fade_pos;
          int l=fade_len/4;
          for (x = 0; x < l; x ++)
          {
            double d1;
            d1=(double)p/(double)buf.getSize();
            double d;
            d=*a * d1 + *b * (1.0-d1);
            int r=(int)d;
            if (r < -32768) r=-32768;
            if (r > 32767) r=32767;
            *a++ = (short) r; b++;

            d=*a * d1 + *b * (1.0-d1);
            r=(int)d;
            if (r < -32768) r=-32768;
            if (r > 32767) r=32767;
            *a++ = (short) r; b++;
            p++;
          }
        }

        fade_pos+=fade_len;

        ASSERT(!(fade_len < numsamples && buf.bytesInBuf() != 0));

        chunk_list->setChunk("VISDATA",NULL,0);
        return 1;
      }      
    }

    // check if we are currently playing crossfading zones
    // otherwise shrink crossfading buffer
    int len=infos->getLength();
    int pos=api->core_getPosition(m_coretoken);
    if(!enabled()/* || pos<(len-(crossfade_time*3*1000))*/)
    {
      int a=buf.bytesInBuf();
      if (!a) return 1;
  
      if (a > (numsamples*2)) a=numsamples*2;
      resizeTempBuffer(a);
      buf.getBytes(tmpbuf,a);
      chunk_list->setChunk("PCM",tmpbuf,a);
      buf.putBytes(s, numsamples);
      chunk_list->setChunk("VISDATA",NULL,0);
      return 1;
    }

    int length_out=(buf.bytesInBuf()+numsamples)-buf.getSize();
    {
      // makes sure that numsamples/2 is in sync with bps and nch
      // (was   if (length_out < numsamples/2) length_out=numsamples/2;)
      int r=(bps==16?2:1)*nch;
      int a=numsamples/r;
      int m=(a/2)*r;
      length_out=MAX(length_out,m);
    }
    length_out=MIN(length_out,buf.bytesInBuf());
    
    resizeTempBuffer(numsamples);
    memcpy(tmpbuf,s,numsamples);
    if (length_out)
    {
      length_out=buf.getBytes(s,length_out);
      chunk_list->setChunk("PCM",s,length_out);
      chunk_list->setChunk("VISDATA",NULL,0);
    } else chunk_list->setChunk("PCM",NULL,0);
    buf.putBytes(tmpbuf,numsamples);

    return 1;
  }

  return 1;
}

void Crossfade::resizeTempBuffer(int size)
{
  if(size>tmpbufsize)
  {
    tmpbuf=(char *)REALLOC(tmpbuf,size);
    tmpbufsize=size;
  }
}

int Crossfade::getLatency(void) {
  return 0;
  if (!buf.isOpen() || !last_nch || !last_bps || !last_srate) return 0;
  return MulDiv(buf.bytesInBuf(),1000,last_nch*last_srate*(last_bps>>3));
}

int Crossfade::corecb_onEndOfDecode() {
  if (enabled()) fade_pos=0;
  return 0;
}

int Crossfade::corecb_onAbortCurrentSong() {
  if (enabled() && !crossfade_on_next) {
    restart();
    return 0;
  }
  if (enabled()) return 1;
  return 0;
}

int Crossfade::enabled() {
  if (ci == NULL) {
    // {FC3EAF78-C66E-4ed2-A0AA-1494DFCC13FF}
    const GUID audio_options_guid = 
    { 0xfc3eaf78, 0xc66e, 0x4ed2, { 0xa0, 0xaa, 0x14, 0x94, 0xdf, 0xcc, 0x13, 0xff } };
    ci = WASABI_API_CONFIG->config_getCfgItemByGuid(audio_options_guid);
  }
  if (ci == NULL) return 0;
  return _intVal(ci, "Enable crossfading");
}
