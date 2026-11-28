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
#include "pcmdsound.h"
#include "../attribs/cfgitemi.h"
#include "../attribs/attrint.h"

unsigned long tea_key[4]={0xef542687,0x4d5c68ac,0x54274ef9,0x844dfc52};
unsigned long tea_sum=0xC6EF3720;
unsigned long tea_delta=0x9E3779B9;

static int strings_decrypt=0;
static char crypted_bps[]={'b'^0x25,'p'^0x25,'s'^0x25,0};
static char crypted_srate[]={(char)('s'+0x41),(char)('r'+0x41),(char)('a'+0x41),(char)('t'+0x41),(char)('e'+0x41),0};
static char crypted_nch[]={'n'-0x18,'c'-0x18,'h'-0x18,0};

static int __voltodb(int v)
{
#if 0
  if (v < 16)       v = ((v-0) * (10000-6000))/15 - 10000;
  else if (v < 32)  v = ((v-16) * (6000-4000))/15 - 6000;
  else if (v < 64)  v = ((v-32) * (4000-2000))/31 - 4000;
  else if (v < 128) v = ((v-64) * (2000-1000))/63 - 2000;
  else if (v < 192) v = ((v-128) * (1000-500))/63 - 1000;
  else if (v < 224) v = ((v-192) * (500-250))/31 - 500;
  else if (v < 240) v = ((v-224) * (250-125))/15 - 250;
  else              v = ((v-240) * (125-0))/15 - 125;
  return v;
#elif 0
  double p_vol=(v+1)/256.0;
  double d=log10(p_vol)*10.0;
  int r=(int)(d*100.0);
  return r;
#else
  static const short tab[256]={
  10000,8085,7074,6482,6063,5737,5471,5246,5051,4879,4725,4586,4459,4343,4234,
  4134,4040,3951,3868,3789,3714,3643,3575,3510,3448,3388,3331,3276,3223,3172,
  3122,3074,3028,2983,2940,2897,2856,2816,2777,2739,2703,2667,2631,2597,2563,
  2531,2499,2467,2437,2406,2377,2348,2320,2292,2265,2238,2212,2186,2160,2135,
  2111,2087,2063,2040,2017,1994,1972,1950,1928,1907,1886,1865,1845,1825,1805,
  1785,1766,1747,1728,1709,1691,1673,1655,1637,1620,1603,1586,1569,1552,1536,
  1519,1503,1487,1471,1456,1440,1425,1410,1395,1380,1365,1351,1337,1322,1308,
  1294,1280,1267,1253,1240,1226,1213,1200,1187,1174,1162,1149,1136,1124,1112,
  1099,1087,1075,1063,1052,1040,1028,1017,1005,994,983,971,960,949,938,
  928,917,906,895,885,874,864,854,844,833,823,813,803,793,784,
  774,764,754,745,735,726,717,707,698,689,680,671,662,653,644,
  635,626,617,608,600,591,583,574,566,557,549,541,532,524,516,
  508,500,492,484,476,468,460,452,444,437,429,421,414,406,398,
  391,383,376,369,361,354,347,339,332,325,318,311,304,297,290,
  283,276,269,262,255,248,242,235,228,222,215,208,202,195,189,
  182,176,169,163,156,150,144,137,131,125,119,112,106,100,94,
  88,82,76,70,64,58,52,46,40,34,28,23,17,11,5,
  0,
  };
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return -tab[v];
#endif
}


int PcmDSound::bufPutBytes(char *p, int n) {
  int put = n;
  int l2;
  l2 = (cbuffer+_cbufferlen)-head;
  if (l2 <= n)
  {
	  memcpy(head,p,l2);
	  head=cbuffer;
	  p+=l2;
	  n-=l2;
  }
  if (n)
  {
	  memcpy(head,p,n);
	  head+=n;
	  p+=n;
  }
  bytes_in_buf += put;
  return put;
}

int PcmDSound::bufGetBytes(char *p, int n) {
  int got = 0;
  if (bytes_in_buf <= 0) 
  {
	  return 0;
  }
  if (n > bytes_in_buf) n=bytes_in_buf;
  bytes_in_buf-=n;
  got=n;
  if (tail+n >= cbuffer+_cbufferlen)
  {
	  int l1=cbuffer+_cbufferlen-tail;
	  n-=l1;
	  memcpy(p,tail,l1);
	  tail=cbuffer;
	  p+=l1;
	  memcpy(p,tail,n);
	  tail+=n;
  } 
  else 
  {
	  memcpy(p,tail,n);
	  tail+=n;
  }
  return got;
}


int PcmDSound::bufClose(void) {
  GlobalFree(cbuffer);
  cbuffer = NULL;
  head = tail = cbuffer;
  bytes_in_buf = 0;
  return 1;
}



PcmDSound::PcmDSound() {
//  m_hwnd=GetDesktopWindow();
  m_hwnd=FindWindow("STUDIO",NULL);
  bInitted=0;
  paused=0;
  second_started=0;
  buffer_size=0;
  last_volume=last_pan=-666;
  last_samplerate=last_nchannels=last_bits=0;
  lpDS=NULL;
  primary=second=NULL;
  bytes_per_sample=0;
  thread_id=NULL;
  time_to_die=0;
  write_pos=0;
  play_pos=0;
  buf_write_pos=0;
	ds_initialized=0;

  last_pause=0;
  g_underrunning=0;
  g_needstart=0;
  playing=0;
  play_pos=0;
  bytes_in_dbuf=0;

  if(!strings_decrypt) {
    int i;
    for(i=0;i<3;i++) crypted_bps[i]^=0x25;
    for(i=0;i<5;i++) crypted_srate[i]-=0x41;
    for(i=0;i<3;i++) crypted_nch[i]+=0x18;
    strings_decrypt=1;
  }
}

PcmDSound::~PcmDSound() {
  ShutDown();
}

int PcmDSound::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  // strings "encrypted" for WMA pcm "secure" stuff
  char pcmstr[5]={(char)('p'+23),'c'^12,'M'-64,0};

  pcmstr[4]=0;
  pcmstr[1]^=12;
  pcmstr[0]-=23;
  pcmstr[2]+=64;
  Chunk *chunk1=chunk_list->getChunk(pcmstr/*"PCM"*/);
  pcmstr[3]=(char)('x'+85);
  pcmstr[3]-=85;
  Chunk *chunk=chunk_list->getChunk(pcmstr/*"PCMx"*/);
  if(chunk) {
    // decrypt using TEA (128-bit)
    int i=chunk->getSize()/8;
    unsigned long *v=(unsigned long *)chunk->getData();
    const unsigned long *k=tea_key;
    while(i) {
      register unsigned long y=v[0],z=v[1],sum=tea_sum, delta=tea_delta,n=32;
      /* sum = delta<<5, in general sum = delta * n */
      while(n-->0) {
        z -= (y << 4 ^ y >> 5) + y ^ sum + k[sum>>11 & 3];
        sum -= delta;
        y -= (z << 4 ^ z >> 5) + z ^ sum + k[sum&3];
      }
      v[0]=y; v[1]=z; v+=2; i--;
    }
  } else {
    chunk=chunk1;
    if(!chunk) return 0;
  }

  int bytes=chunk->getSize();
  if(!bytes) return 1;

  int bps=chunk->getInfo(crypted_bps);
  int nch=chunk->getInfo(crypted_nch);
  int srate=chunk->getInfo(crypted_srate);
  char *sampledata=(char *)chunk->getData();

  if ((bps != 8 && bps != 16) || (nch != 1 && nch != 2) || !srate) return 0;

  m_infos=infos;
  if (!bInitted)
  {
    if (do_open() < 0)
      return 0;
    SetVolume(api->core_getVolume(0));
    bInitted=1;
  }
  int newlen=bytes;

  while (newlen > 0)
  {
    int r=ResampleAndWrite(sampledata,newlen,bps,nch,srate,killswitch);
    newlen-=r;
    sampledata+=r;
  }

  return 1;
}

int PcmDSound::getLatency(void) {
  if (bInitted) {
    int r=do_getouttime()-do_getwritetime();
/*    if(r>5000) {
      int a=1;
      r=5000;
    }*/
    return r;
  }
  return 0;
}

int PcmDSound::ResampleAndWrite(char *insamples, int bytes, int bps, int nch, int srate, bool *killswitch)
{                 
  char buf[8192];
  ASSERT(bytes > 0);
	if (nch == 2 && bps == 16) 
    // samples are correct size and channels. simple resample (working fine)
	{
		if (srate == 44100) 
    {
      int n=min(bytes,8192);
      do_write(insamples,n,killswitch);
      return n;
    }
		else 
    {
			int *in=(int*)insamples;
			int *out=(int*)buf;
			int r=0;
			int dr=(srate<<12)/44100;

			int newlen=min(8192/4,MulDiv(bytes/4,44100,srate));
			int x = newlen;
			while (x--)
			{
				*out++=in[r>>12];
				r+=dr;
			}
			do_write(buf,newlen*4,killswitch);
      return MulDiv(newlen,srate,44100)*4;
		}
	}
  else // resample and convert sample types
	{
    if (srate == 44100 && bps == 16 && nch == 1)
    {
			short *in=(short*)insamples;
			short *out=(short*)buf;

			int newlen=min(8192/4,bytes/2);
			int x = newlen;
			while (x--)
			{
				*out++=*in;
				*out++=*in++;
			}
			do_write(buf,newlen*4,killswitch);
      return newlen*2;
    }
    else
    {
		  short *out=(short *)buf;
		  int r=0;
		  int dr=((srate<<12)/44100);

		  int newlen=min(MulDiv((bytes / (bps/8)) / nch,44100,srate),8192/4);
      int x = newlen;

      if (nch == 1)
      {
		    while (x--)
		    {
			    short spl;
			    if (bps == 8) 
			    {
				    int p=insamples[r>>12];
				    spl=(p^128)<<8;
			    } 
			    else spl=((short *)insamples)[r>>12];
			    *out++=spl;
			    *out++=spl;
			    r+=dr;
		    }
      }
      else
      {
		    while (x--)
		    {
			    short spl,spl2;
			    if (bps == 8) 
			    {
				    int p=insamples[(r>>12)<<1];
				    spl=(p^128)<<8;
            p=insamples[((r>>12)<<1)+1];
            spl2=(p^128)<<8;
			    } 
			    else 
			    {
				    spl=((short *)insamples)[(r>>12)<<1];
            spl2=((short *)insamples)[((r>>12)<<1)+1];
			    }
			    *out++=spl;
			    *out++=spl2;
			    r+=dr;
		    }
      }
		  do_write(buf,newlen*4,killswitch);
      return MulDiv(newlen,srate,44100)*(nch*(bps/8));
    }
	}
}


void PcmDSound::ShutDown()
{
  if (bInitted)
  {
    do_close();
    bInitted=0;
    printf("DSound shutdown!\n");
  }
}

int config_cprim=1;
int config_blocklen=32, config_bufferlen=25;
unsigned char config_volumeenabled=1;
unsigned char config_panrev=0;
int config_sleeptime=10;

static GUID zero_guid;

void PcmDSound::Pause(int pause)
{
  if (!playing) return;
  if (pause == paused) return;
  paused = pause;
  EnterCriticalSection(&cs);
  g_underrunning=0;
  if (paused)
  {
		IDirectSoundBuffer_Stop(second);
    g_needstart=write_pos%buffer_size;
  }
  LeaveCriticalSection(&cs);
  return;
}



int PcmDSound::SetVolume(int v)
{
  if (v == -666) v = last_volume;
  if (v == -666) return 1;
  last_volume = v;
  if (!playing || second == NULL) return 1;
  if (!config_volumeenabled) return 1;

  if (v > 255) v = 255;
  if (v < 0) v = 0;

  v=__voltodb(v);

  if (v > 0) v = 0;	
  else if (v < -10000) v = -10000;

  IDirectSoundBuffer_SetVolume(second, v);
  return 1;
}

int PcmDSound::SetPan(int pan)
{
  int v;
	if (pan == -666) pan = last_pan;
	if (pan == -666) return 1;
	last_pan = pan;
  if (!playing || second == NULL) return 1;
  if (!config_volumeenabled) return 1;

  if (config_panrev) pan=-pan;
  {
	  int r=pan<0?1:0;
	  if (pan < 0) pan=-pan;
	  v=pan;
	  v*=2;
	  v=- __voltodb(255-v);
	  if (r) v=-v;
  }
  IDirectSoundBuffer_SetPan(second, v);

  return 1;
}


DWORD WINAPI PcmDSound::_thread(LPVOID lpData) {
  PcmDSound *_this=(PcmDSound *)lpData;
  return _this->thread();
}

int PcmDSound::thread() {
  int bsize=(last_nchannels*(last_bits/8));
	while (1) 
	{
    Sleep(config_sleeptime);
		if (time_to_die) break;
		if (paused) continue;
		EnterCriticalSection(&cs);
    if (!time_to_die) {
      int wrote_bytes=0;
			char *pt1, *pt2;
			int len1, len2, len;
      DWORD pp=0;
      if (g_needstart == -1) IDirectSoundBuffer_GetCurrentPosition(second,&pp,NULL);
      else pp=buffer_size;

      len = pp-write_pos;
      if (len < 0) len += buffer_size;
      bytes_in_dbuf=buffer_size-len;

			if (len > bsize)
      {
        if (bufInBuf())
        {
          HRESULT res;
          g_underrunning=0;
          len&=~(bsize-1);
          res=IDirectSoundBuffer_Lock(second, write_pos,len, 
					    (void **)&pt1, (unsigned long *)&len1, (void **)&pt2, (unsigned long *)&len2, 0);
          if (res == DS_OK) 
			    {
  		      int wrote1=0,wrote2=0;
				    int l=MIN(len1,len);
				    wrote1=bufGetBytes(pt1,l);
				    write_pos+=wrote1;
//            play_pos+=wrote1;
            play_pos+=MulDiv(wrote1,1000*10000,bytes_per_sample*last_samplerate);
            wrote_bytes+=wrote1;
				    if (wrote1==l && pt2)
				    {
					    l=MIN(len2,len-wrote1);
					    wrote2=bufGetBytes(pt2,l);
					    write_pos+=wrote2;
              wrote_bytes+=wrote2;
//              play_pos+=wrote2;
              play_pos+=MulDiv(wrote2,1000*10000,bytes_per_sample*last_samplerate);
				    }
				    IDirectSoundBuffer_Unlock(second, pt1, wrote1, pt2, wrote2);
			    }
          else if (res == DSERR_BUFFERLOST)
          {
            IDirectSoundBuffer_Restore(second);
          }
        }
        else if (g_needstart == -1 && len > (buffer_size*7)/8) // no data available, let's write some zero's
        {
          HRESULT res;
          len/=4;
          len&=~(bsize-1);
          g_underrunning=1;
          res=IDirectSoundBuffer_Lock(second, write_pos,len, 
					    (void **)&pt1, (unsigned long *)&len1, (void **)&pt2, (unsigned long *)&len2, 0);
          if (res == DS_OK) 
			    {
  		      int wrote1=0,wrote2=0;
				    wrote1=MIN(len1,len);
            memset(pt1,(last_bits==8)?0x80:0,wrote1);
				    write_pos+=wrote1;
				    if (pt2)
				    {
					    wrote2=MIN(len2,len-wrote1);
					    memset(pt2,(last_bits==8)?0x80:0,wrote2);
					    write_pos+=wrote2;
				    }
				    IDirectSoundBuffer_Unlock(second, pt1, wrote1, pt2, wrote2);
			    }
          else if (res == DSERR_BUFFERLOST)
          {
            IDirectSoundBuffer_Restore(second);
          }
        }
        if (write_pos >= buffer_size) write_pos-=buffer_size;
      }
      if (g_needstart > -1)
      {
        if (wrote_bytes)
        {
          HRESULT err;
          IDirectSoundBuffer_SetCurrentPosition(second,g_needstart);
          err = IDirectSoundBuffer_Play(second, 0, 0, DSBPLAY_LOOPING);
          if (err == DS_OK)
          {
            g_needstart=-1;
          }
          else if (err == DSERR_BUFFERLOST)
          {
            IDirectSoundBuffer_Restore(second);
          }
        }
      }
    }
		LeaveCriticalSection(&cs);
	}
 
	return 0;
}


int PcmDSound::do_open(void) 
{
  int samplerate=44100, nchannels=2, bits=16;

	int bufs=MulDiv(config_bufferlen*100,samplerate*nchannels*(bits/8),1000);
	int blen=config_blocklen*1024*nchannels*(bits/8);

	if (bufs < 4*1152) bufs=4*1152;

	last_pause=0;

  int buf_size=bufs;
  int ds_size=blen;

  int len1,len2;
  char *pt1,*pt2;
  HRESULT ret;
  DSBUFFERDESC pd={sizeof(pd),};
  WAVEFORMATEX wfx;
  LPGUID lpguid=NULL;

 
  buffer_size=ds_size;

  g_underrunning=0;
  paused=0;
  write_pos=0; 
  play_pos=0;
  buf_write_pos=0;
  g_needstart=0;
  time_to_die = 0;

  last_samplerate = samplerate;
  last_nchannels = nchannels;
  last_bits = bits;

/*  if (memcmp(&zero_guid,&config_curguid,sizeof(config_curguid)))
    lpguid=&config_curguid;*/
  if ((ret = DirectSoundCreate(lpguid, &lpDS, NULL)) != DS_OK) 
  {
    MessageBox(m_hwnd, "Could not create DirectSound object", "Error", MB_OK);
    return -1;
  }

 
   if ((ret = IDirectSound_SetCooperativeLevel(lpDS, m_hwnd, DSSCL_PRIORITY)) != DS_OK) 
   {
      MessageBox(m_hwnd, "Could not set cooperative level", "Error", MB_OK);
      IDirectSound_Release(lpDS);	// release dsound obj
      return -1;
	}

  if (config_cprim&1)
  {
    pd.dwFlags = DSBCAPS_PRIMARYBUFFER;

    ret = IDirectSound_CreateSoundBuffer(lpDS, &pd, &primary, NULL);
    if (ret != DS_OK) 
    {
      MessageBox(m_hwnd, "Could not create primary buffer", "Error", MB_OK);
      IDirectSound_Release(lpDS);	// release dsound obj
      primary = second = (LPDIRECTSOUNDBUFFER)NULL;
      return -1;
    }


    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.cbSize = 0;
    wfx.nChannels = 2;
    if (samplerate > 44100) wfx.nSamplesPerSec = samplerate;
    else wfx.nSamplesPerSec = 44100;
    if (bits > 16) wfx.wBitsPerSample = bits;
    else wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.wBitsPerSample/8)*2;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if ((ret = IDirectSoundBuffer_SetFormat(primary, &wfx)) != DS_OK) {
      if (primary) IDirectSoundBuffer_Release(primary);
      MessageBox(m_hwnd, "Could not set output format", "Error", MB_OK);
      IDirectSound_Release(lpDS);	// release dsound obj, kills all buffers
      primary = second = (LPDIRECTSOUNDBUFFER)NULL;
      return -1;
    }
  }
  else primary=NULL;

  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.cbSize = 0;
 
  wfx.nChannels = nchannels;
  wfx.nSamplesPerSec = samplerate;
  wfx.nAvgBytesPerSec = (bits / 8) * samplerate * nchannels;
  wfx.wBitsPerSample = bits;
  wfx.nBlockAlign = bytes_per_sample = (bits / 8) * nchannels;

  /* now describe our secondary buffer */
  pd.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | 
	           DSBCAPS_GLOBALFOCUS |
               DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_STICKYFOCUS ;
  pd.dwBufferBytes = buffer_size;
  pd.lpwfxFormat = &wfx;

  /* create our secondary buffer */
  ret = IDirectSound_CreateSoundBuffer(lpDS, &pd, &second, NULL);
  if (ret != DS_OK) {
    if (primary) IDirectSoundBuffer_Release(primary);
    MessageBox(m_hwnd, "Could not create secondary buffer", "Error", MB_OK);
    IDirectSound_Release(lpDS);	// release dsound obj, kills all buffers
    primary = second = (LPDIRECTSOUNDBUFFER)NULL;
    return -1;
  }

  if (IDirectSoundBuffer_Lock(second, 0,buffer_size, 
			(void **)&pt1, (unsigned long *)&len1, (void **)&pt2, (unsigned long *)&len2, 0) == DS_OK) 
	{
  	int wrote1=0,wrote2=0;
		wrote1=len1;
    memset(pt1,(last_bits==8)?0x80:0,wrote1);
		if (pt2)
		{
			wrote2=len2;
			memset(pt2,(last_bits==8)?0x80:0,wrote2);
		}
		IDirectSoundBuffer_Unlock(second, pt1, wrote1, pt2, wrote2);
	}

  if (primary && (config_cprim & 2))
  {
    int n=10;
    HRESULT res=0;
    while (n--)
    {
      res=IDirectSoundBuffer_Play(primary, 0, 0, DSBPLAY_LOOPING);
      if (res == DS_OK) break;
      if (res == DSERR_BUFFERLOST)
      {
        IDirectSoundBuffer_Restore(primary);
        Sleep(50);
      }
    }
    if (res != DS_OK) {
      MessageBox(m_hwnd, "Could not start primary buffer", "Error", MB_OK);
      if (second) IDirectSoundBuffer_Release(second);
      if (primary) IDirectSoundBuffer_Release(primary);
      IDirectSound_Release(lpDS);	// release dsound obj, kills all buffers
      primary = second = (LPDIRECTSOUNDBUFFER)NULL;
      return -1;
    }
  }

  bufOpen(buf_size);
  InitializeCriticalSection(&cs);

  playing = 1;
  {
	  DWORD id;
	  thread_id = CreateThread(NULL,0, _thread, this,0, &id);
  	SetThreadPriority(thread_id,THREAD_PRIORITY_TIME_CRITICAL);
  }
  return 0;
}


int PcmDSound::do_close(void) {
  if (!playing) return 0;

	EnterCriticalSection(&cs);
  time_to_die = 1;
  if (g_needstart == -1)
  {
    IDirectSoundBuffer_Stop(second);
  }
  if (primary) IDirectSoundBuffer_Stop(primary);
	LeaveCriticalSection(&cs);

  if (thread_id) 
  {
    WaitForSingleObject(thread_id,INFINITE);
    CloseHandle(thread_id);
  }
  thread_id=NULL;

  if (second) IDirectSoundBuffer_Release(second);
  if (primary) IDirectSoundBuffer_Release(primary);
  IDirectSound_Release(lpDS);
  lpDS = NULL;
  primary = second = (LPDIRECTSOUNDBUFFER)NULL;
  playing = 0;

  bufClose();

  DeleteCriticalSection(&cs);

  return 0;
}


int PcmDSound::do_write(char *buf, int len, bool *killswitch) 
{
  int rlen=len;
  if (!playing) return -1;
  if (!len || !buf)
  {
	  return 0;
  }

  while(len && !*killswitch) {
    EnterCriticalSection(&cs);
    int r=bufAvailable();
    if(!r) Sleep(10);
    else {
      if(r>len) r=len;
      //buf_write_pos += r/8/bytes_per_sample;
      buf_write_pos+=MulDiv(r,1000*10000,bytes_per_sample*last_samplerate);
      bufPutBytes(buf,r);
      buf+=r;
      len-=r;
    }
    LeaveCriticalSection(&cs);
  }
//  buf_write_pos += rlen/8/bytes_per_sample;
  return 0;
}

int PcmDSound::do_getouttime(void) {
//  int r,l=0;
  if (!playing) return 0;
//  r=play_pos-bytes_in_dbuf;
//  return MulDiv(r/bytes_per_sample,1000,last_samplerate);
  return (int)(play_pos/10000)-MulDiv(bytes_in_dbuf/bytes_per_sample,1000,last_samplerate);
}

int PcmDSound::do_getwritetime(void) {
  if (!playing) return 0;
//  return MulDiv(buf_write_pos,8000,last_samplerate);
  return (int)(buf_write_pos/10000);
}

void PcmDSound::do_flush(void) {
  unsigned long len1,len2;
  char *pt1,*pt2;
  if (!playing) return;
  EnterCriticalSection(&cs);
  g_underrunning=0;
  IDirectSoundBuffer_Stop(second);
  bufReset();
  write_pos=0;
  play_pos=0;
  buf_write_pos=0;
  if (IDirectSoundBuffer_Lock(second, 0,buffer_size, 
			(void **)&pt1, &len1, (void **)&pt2, &len2, 0) == DS_OK) 
	{
  	int wrote1=0,wrote2=0;
		wrote1=len1;
    memset(pt1,(last_bits==8)?0x80:0,wrote1);
		if (pt2)
		{
			wrote2=len2;
			memset(pt2,(last_bits==8)?0x80:0,wrote2);
		}
		IDirectSoundBuffer_Unlock(second, pt1, wrote1, pt2, wrote2);
	}
  g_needstart=0;
  LeaveCriticalSection(&cs);
}
