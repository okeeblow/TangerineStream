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
#include "pcmwaveout.h"
#include "cnv_pcmwaveout.h"
#include "config.h"

#define S_MINSIZE (1<<28)

Config config;

unsigned long tea_key[4]={0xef542687,0x4d5c68ac,0x54274ef9,0x844dfc52};
unsigned long tea_sum=0xC6EF3720;
unsigned long tea_delta=0x9E3779B9;

static int strings_decrypt=0;
static char crypted_bps[]={'b'^0x25,'p'^0x25,'s'^0x25,0};
static char crypted_srate[]={(char)('s'+0x41),(char)('r'+0x41),(char)('a'+0x41),(char)('t'+0x41),(char)('e'+0x41),0};
static char crypted_nch[]={'n'-0x18,'c'-0x18,'h'-0x18,0};

PcmWaveout::PcmWaveout()
{
  bInitted=0;
  m_min_blocksize=0;
  m_max_blocksize=0;
  m_num_blocks=0;
  m_buffer=m_buffer_write=m_buffer_read=NULL;
  m_buffer_length=m_buffer_valid=m_buffer_inlength=0;
  m_hWaveOut=NULL;
  memset(m_wave_headers,0,sizeof(m_wave_headers));
  m_pause=m_writeall=m_wavecnt=m_prebuf=0;
  m_quit_flag=0;
  m_hSem=m_hEvent=m_hThread=NULL;
  m_bytes_per_sec=m_outtime_interp=0;
  m_outtime=m_writetime=0;
  a_v=255;a_p=0;
  vol_tab=NULL;
  use_vol_tab=0;

  SetVolume(api->core_getVolume(0));

  if(!strings_decrypt) {
    int i;
    for(i=0;i<3;i++) crypted_bps[i]^=0x25;
    for(i=0;i<5;i++) crypted_srate[i]-=0x41;
    for(i=0;i<3;i++) crypted_nch[i]+=0x18;
    strings_decrypt=1;
  }
}

PcmWaveout::~PcmWaveout()
{
  ShutDown();
}

int PcmWaveout::ResampleAndWrite(char *insamples, int bytes, int bps, int nch, int srate, bool *killswitch)
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

int PcmWaveout::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
{
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

  char *sampledata=(char *)chunk->getData();
  int bps=chunk->getInfo(crypted_bps);
  int srate=chunk->getInfo(crypted_srate);
  int nch=chunk->getInfo(crypted_nch);

  if ((bps != 8 && bps != 16) || !srate || (nch != 1 && nch != 2)) return 0;
  if (!bInitted)
  {
    if (do_open() < 0)
      return -1;
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

void PcmWaveout::ShutDown()
{
  if (bInitted)
  {
    bInitted=0;
    do_close();
    printf("WaveOut shutdown!\n");
  }
}

static void CALLBACK cbFunc(HWAVEOUT hwo,UINT uMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2)
{
	if (uMsg == WOM_DONE) { ReleaseSemaphore((HANDLE)dwInstance,1,NULL);}
}

int PcmWaveout::do_open(void)
{
	int x;
	DWORD id;
	MMRESULT res;
	WAVEFORMATEX wfx={WAVE_FORMAT_PCM,2,44100,44100*2*(16/8),2*(16/8),16};
	  
  printf("WaveOut started!\n");
  m_bytes_per_sec=wfx.nAvgBytesPerSec;

	memset(&m_wave_headers,0,sizeof(m_wave_headers));

	m_buffer_length = MulDiv(m_bytes_per_sec, config.bufsize_ms, 1000);
	m_buffer_length &= ~1023;
	if (m_buffer_length < 4096) m_buffer_length=4096;

	m_buffer=(char *)malloc(m_buffer_length);
	if (m_buffer == NULL)
	{
//    m_player->Error("waveOut: error allocating buffer"); 
		return -1;
	}

	m_prebuf=m_buffer_length/4;
	m_buffer_read=m_buffer_write=m_buffer;
	m_wavecnt=m_pause=m_writeall=m_buffer_valid=m_buffer_inlength=0;
  m_outtime=m_writetime=0;
	m_quit_flag=0;
	
	do_set_blocksizes();

	m_hSem=CreateSemaphore(NULL,0,256,NULL);
	for (x = 0; (res=waveOutOpen(&m_hWaveOut,config.out_dev,&wfx,(DWORD)cbFunc,(DWORD)m_hSem,CALLBACK_FUNCTION))==MMSYSERR_ALLOCATED && x<10; x ++)
		Sleep(500);
	if (res != MMSYSERR_NOERROR)
	{
		char t[512];
		waveOutGetErrorText(res,t,sizeof(t));
//    m_player->Error(t); 
		free(m_buffer);
		CloseHandle(m_hSem);
		m_buffer = NULL;
		return -1;
	}

	m_outtime_interp=GetTickCount();
	m_hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
	InitializeCriticalSection(&m_cs);

	SetVolume(-666);

	m_hThread=CreateThread(NULL,0,_threadproc,this,0,&id);
	do_set_priority();

	return MulDiv(m_buffer_length,1000,m_bytes_per_sec);
}

void PcmWaveout::do_set_blocksizes(void)
{
	int t,t2,t4;
	t=(MulDiv(config.blocksize_min,m_bytes_per_sec,44100)+1023)&~1023;
	if (t<1024) t=1024;
	if (t>32768) t=32768;

	t2=(MulDiv(config.blocksize_max,m_bytes_per_sec,44100*4)+1023)&~1023;
	if (t2 < t) t2 = t;
	if (t2 > 65536) t2=65536;

	t4 = config.num_blocks;
	if (t4 < 2) t4=2;
	if (t4 > MAX_NUM_BLOCKS) t4=MAX_NUM_BLOCKS;

	m_num_blocks=t4;
	m_max_blocksize=t2;
	m_min_blocksize=t;
}

void PcmWaveout::do_set_priority(void)
{
	int priorities[]=
		{THREAD_PRIORITY_NORMAL,THREAD_PRIORITY_ABOVE_NORMAL,THREAD_PRIORITY_HIGHEST,THREAD_PRIORITY_TIME_CRITICAL};
	if (m_hThread) SetThreadPriority(m_hThread,priorities[MAX(0,MIN((int)config.priority,3))]);
}

void PcmWaveout::do_close(void)
{
	int x;
	m_quit_flag=1;
	SetEvent(m_hEvent);
	while (m_quit_flag == 1) Sleep(70);

	waveOutReset(m_hWaveOut);

	for (x = 0; x < MAX_NUM_BLOCKS; x++)
		if (m_wave_headers[x].dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(m_hWaveOut,&m_wave_headers[x],sizeof(m_wave_headers[0]));
	if (waveOutClose(m_hWaveOut) != MMSYSERR_NOERROR)
	{
//    m_player->Warning("Error closing sound device"); 
	}
	if (m_buffer) free(m_buffer);
	DeleteCriticalSection(&m_cs);
	CloseHandle(m_hThread);
	CloseHandle(m_hSem);
	CloseHandle(m_hEvent);
	m_hThread=m_hSem=m_hEvent=NULL;
  m_buffer=NULL;
  m_hWaveOut=NULL;
	m_wavecnt=m_pause=m_writeall=m_buffer_valid=
		m_buffer_inlength=m_buffer_length=0;
  m_outtime=m_writetime=0;
}

int PcmWaveout::do_write(char *buf, int len, bool *killswitch)
{
	int l2;

  while (do_canwrite() < len && !*killswitch) Sleep(10);

	if (len > 8192) len=8192;
	l2=(m_buffer_write+len)-(m_buffer+m_buffer_length);
	if (len <= 0 || !buf)
	{
		m_writeall=1;
		SetEvent(m_hEvent);
		return 0;
	}
	m_writeall=0;
	if (l2 > 0)
	{
		int l1=len-l2;
		memcpy(m_buffer_write,buf,l1);
		memcpy(m_buffer,buf+l1,l2);
		m_buffer_write=m_buffer+l2;
	}
	else
	{
		memcpy(m_buffer_write,buf,len);
		m_buffer_write += len;
		if (m_buffer_write == m_buffer+m_buffer_length) m_buffer_write=m_buffer;
	}

	EnterCriticalSection(&m_cs);
	m_buffer_valid+=len;
	LeaveCriticalSection(&m_cs);
//	m_writetime_bytes+=len;
  m_writetime+=MulDiv(len,1000*10000,m_bytes_per_sec);
	if (m_wavecnt < m_num_blocks) 
	{
		SetEvent(m_hEvent);
	}
	return 0;
}

int PcmWaveout::do_canwrite(void)
{
	int t=(m_buffer_length-m_buffer_valid);
	if (m_wavecnt==0) 
	{
		SetEvent(m_hEvent);
	}
	return t;
}

int PcmWaveout::do_isplaying(void)
{
	if (m_wavecnt==0) 
	{
		SetEvent(m_hEvent);
	}
	m_writeall=1;
	return (m_buffer_valid>0?1:0);
}

void PcmWaveout::Pause(int pause)
{
	if (m_hWaveOut)
	{
		int lastp=m_pause;
		m_pause=pause;
		if (pause == lastp) return;
		if (m_pause)
			waveOutPause(m_hWaveOut);
		else
		{
			waveOutRestart(m_hWaveOut);
			m_outtime_interp=GetTickCount();
			SetEvent(m_hEvent);
		}
	}
}


void PcmWaveout::_setvol(void)
{
	DWORD vol, vo2, gv;
	if (m_hWaveOut)
	{
		a_v=MIN(255,MAX(a_v,0));
		a_p=MIN(127,MAX(a_p,-127));

		vo2 = vol = (a_v*65535) / 255;

		if (a_p > 0)
		{
			vol *= (127-a_p);
			vol /= 127;
		}
		else if (a_p < 0)
		{
			vo2 *= (127+a_p);
			vo2 /= 127;
		}
		gv=vol|(vo2<<16);
		if (config.volflags&2) gv=((gv>>16)&0xffff)|((gv<<16)&0xffff0000);
		if (config.volflags&1)
		{
			if (!(config.volflags&4)) 
			{
				if (use_vol_tab)
				{
					EnterCriticalSection(&m_cs);
					free(vol_tab);
					vol_tab=NULL;
					use_vol_tab=0;
					LeaveCriticalSection(&m_cs);
				}
				waveOutSetVolume(m_hWaveOut,gv);
			}
			else
			{
				int x;
				EnterCriticalSection(&m_cs);
				if (!use_vol_tab)
				{
					int l=(4*32769);
					vol_tab=(unsigned char *)malloc(l);
          memset(vol_tab,0,l);
					waveOutSetVolume(m_hWaveOut,0xffffffff);
				}
				use_vol_tab=0;
				{
					short *vol_tab16 = (short *)vol_tab;
					for (x = 0; x < 32769; x ++)
						{
							vol_tab16[x]       = - (signed) ((x*vol)/65536);
							vol_tab16[x+32769] = - (signed) ((x*vo2)/65536);
						}
				} 
				use_vol_tab=1;
				LeaveCriticalSection(&m_cs);
			}
		}
	}
}

int PcmWaveout::SetVolume(int volume)
{
	if (volume != -666) a_v=volume;
	_setvol();
  return 1;
}

int PcmWaveout::SetPan(int pan)
{
	if (pan != -666) a_p=pan;
	_setvol();
  return 1;
}

void PcmWaveout::do_flush(void)
{
	int x;
	EnterCriticalSection(&m_cs);
	m_outtime=m_writetime=0;
	waveOutReset(m_hWaveOut);
	for (x = 0; x < MAX_NUM_BLOCKS; x++)
		if ((m_wave_headers[x].dwFlags & WHDR_PREPARED))
		{
			waveOutUnprepareHeader(m_hWaveOut,&m_wave_headers[x],sizeof(m_wave_headers[0]));
			m_wave_headers[x].dwFlags =0;
		}
	while (WaitForSingleObject(m_hSem,0)==WAIT_OBJECT_0);
	m_prebuf=m_buffer_length/8;
	m_buffer_read=m_buffer_write=m_buffer;
	m_writeall=m_wavecnt=m_buffer_valid=m_buffer_inlength=0;
	LeaveCriticalSection(&m_cs);
}

int PcmWaveout::do_getouttime(void)
{
	int t=0;
  if (!m_pause) 
	{
		t=GetTickCount()-m_outtime_interp;
		if (t > 1000) t=1000;
	}
	return t+(int)(m_outtime/10000); //MulDiv(m_outtime_bytes,1000,m_bytes_per_sec);
}

int PcmWaveout::do_getwritetime(void)
{
//	return MulDiv(m_writetime_bytes,1000,m_bytes_per_sec);
  return (int)(m_writetime/10000);
}

void PcmWaveout::do_samples_altvol(char *in, int blen)
{
	if ((a_v != 255 || a_p) && use_vol_tab) 
	{
		EnterCriticalSection(&m_cs);
		if (use_vol_tab) 
		{
    	{
				short *i = (short *) in;
				short *tab= (short *)vol_tab;
				int x = blen>>1;
				{
					x>>=1;
					while (x--)
					{
						int a = i[0];
						if (a <= 0)	i[0] = tab[-a];
						else i[0] = -tab[a];
						a=i[1];
						if (a <= 0)	i[1] = tab[32769-a];
						else i[1] = -tab[32769+a];
						i+=2;
					}
				}
			}
		}
		LeaveCriticalSection(&m_cs);
	}
}



DWORD WINAPI PcmWaveout::_threadproc(LPVOID p)
{
  return ((PcmWaveout *)p)->ThreadProc();
}

DWORD PcmWaveout::ThreadProc(void)
{
	HANDLE hs[2]={m_hSem,m_hEvent};
	while (1) 
	{
		int i;
		i=WaitForMultipleObjects(2,hs,FALSE,INFINITE);
		if (m_quit_flag) break;
		if (i == WAIT_OBJECT_0)
		{
			int x;
			for (x = 0; x < MAX_NUM_BLOCKS && !(m_wave_headers[x].dwFlags & WHDR_DONE); x++);
			if (x < MAX_NUM_BLOCKS) 
			{
				EnterCriticalSection(&m_cs);
				if (m_wave_headers[x].dwFlags & WHDR_DONE)
				{
					int r=m_wave_headers[x].dwBufferLength;
					m_outtime_interp=GetTickCount();
					m_buffer_valid   -=r;
					m_buffer_inlength-=r;
//					m_outtime_bytes  +=r;
          m_outtime+=MulDiv(r,1000*10000,m_bytes_per_sec);
					waveOutUnprepareHeader(m_hWaveOut,&m_wave_headers[x],sizeof(m_wave_headers[0]));
					m_wave_headers[x].dwFlags=0;
					m_wavecnt--;
					i++;
				}
				LeaveCriticalSection(&m_cs);
			}
		}
		if (i == WAIT_OBJECT_0+1 && !m_pause)
		{
			int l;
			int m;
			int t=m_num_blocks;
			EnterCriticalSection(&m_cs);
			l=m_buffer_valid-m_buffer_inlength;
			LeaveCriticalSection(&m_cs);
		again:
			if (m_quit_flag) break;
			if (m_writeall) m=1;
			else
			{
				m=MAX(MIN(m_buffer_length/2,m_min_blocksize),m_prebuf);
				l&=~1023;
			}
			if (l >= m && m_wavecnt < t)
			{
				int x;
				if (l > m_max_blocksize) l=m_max_blocksize;
				for (x = 0; x < t && (m_wave_headers[x].dwFlags & WHDR_PREPARED); x++);
				if (x < t)
				{
					int ml=(m_buffer+m_buffer_length)-m_buffer_read;
					if (l>ml) l=ml;

					m_wave_headers[x].dwBufferLength=l;
					m_wave_headers[x].lpData=m_buffer_read;
					if (waveOutPrepareHeader(m_hWaveOut,&m_wave_headers[x],sizeof(m_wave_headers[0])) == MMSYSERR_NOERROR)
					{
						do_samples_altvol(m_wave_headers[x].lpData,m_wave_headers[x].dwBufferLength);
						if (waveOutWrite(m_hWaveOut,&m_wave_headers[x],sizeof(m_wave_headers[0])) == MMSYSERR_NOERROR)
						{
							m_prebuf=0;

							m_wavecnt++;

							m_buffer_inlength += l;
							m_buffer_read += l;
							
							if (m_buffer_read >= m_buffer+m_buffer_length) m_buffer_read=m_buffer;

							if (m_wavecnt < t)
							{
								EnterCriticalSection(&m_cs);
								l=m_buffer_valid-m_buffer_inlength;
								LeaveCriticalSection(&m_cs);
								if (l >= m) goto again;
							}
						}
						else 
						{
							waveOutUnprepareHeader(m_hWaveOut,&m_wave_headers[x],sizeof(m_wave_headers[0]));
							m_wave_headers[x].dwFlags=0;
						}
					}
					else m_wave_headers[x].dwFlags=0;
				}
			} 
		}
	}
	m_quit_flag=2;
	return 0;
}
