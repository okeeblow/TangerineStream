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
#include "main.h"
#include "ds2.h"
#include <dsound.h>
#include <math.h>
#include <ks.h>
#include <ksmedia.h>

static UINT fmt_sr,fmt_nch,fmt_bps;


GUID cfg_dev2;
UINT cfg_buf_ms=2000,cfg_prebuf2=500,cfg_sil_db=400,cfg_trackhack=200;
bool cfg_killsil=0;
bool cfg_wait=1;
bool cfg_createprimary=0;
bool cfg_volume=1;


#ifdef HAVE_SSRC

#include "ssrc/ssrc.h"

static Resampler_base* pSSRC;


UINT cfg_dither=1,cfg_resample_freq=48000,cfg_resample_bps=1;
bool cfg_fast=1;
UINT cfg_pdf=1;
UINT bps_tab[3]={8,16,24};
static UINT resample_freq;
static UINT resample_bps;
#define KILL_SSRC {if (pSSRC) {delete pSSRC;pSSRC=0;}}

static bool finished,use_finish;

static void do_ssrc_create()
{
	pSSRC=SSRC_create(fmt_sr,resample_freq,fmt_bps,resample_bps,fmt_nch,cfg_dither,cfg_pdf,cfg_fast);
	if (!pSSRC)
	{
		resample_freq=fmt_sr;
		resample_bps=fmt_bps;
	}
	else
	{
		finished=0;
		use_finish=cfg_trackhack==0 ? 1 : 0;
	}
}


#else
#define KILL_SSRC 
#endif
static CRITICAL_SECTION sync;
#define SYNC_IN EnterCriticalSection(&sync);
#define SYNC_OUT LeaveCriticalSection(&sync);


//#define USE_LOG

#ifdef _DEBUG
#define USE_LOG
#endif

#ifdef USE_LOG

static void log_write(char* msg)
{
	char tmp[512];
	SYSTEMTIME st;
	GetSystemTime(&st);
	wsprintf(tmp,"wa2: %u:%02u:%02u.%03u   %s\n",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds, msg);
	OutputDebugString(tmp);
}

#else

#define log_write(x)

#endif


static UINT fadetimehack;

static int wa2_hint;
enum
{
	HINT_NONE,HINT_EOF,HINT_EOF_GAPLESS
};


void Config(HWND w);

void About(HWND p)
{
#ifdef HAVE_SSRC
	MessageBox(p,NAME"\n(c) 2001-2002 Peter Pawlowski\nhttp://www.blorp.com/~peter/\n\nSSRC resampling code written by Naoki Shibata\nhttp://shibatch.sourceforge.net/\n\nBuilt on: "__TIMESTAMP__,"About",MB_ICONINFORMATION);
#else
	MessageBox(p,NAME"\n(c) 2001-2002 Peter Pawlowski\nhttp://www.blorp.com/~peter/\n\nBuilt on: "__TIMESTAMP__,"About",MB_ICONINFORMATION);
#endif
}

void do_cfg(bool);

#define malloc(x) LocalAlloc(LMEM_FIXED,x)
#define free(x) LocalFree(x)

static DS2* pDS2;

void Init()
{
	InitializeCriticalSection(&sync);

	if (GetVersion()&0x80000000) cfg_createprimary=1;//win9x

	do_cfg(0);
}


void Quit()
{
	if (pDS2)
	{
		pDS2->Release();
		pDS2=0;
	}
	KILL_SSRC;

	DS2::quit(cfg_wait);
	do_cfg(1);
	DeleteCriticalSection(&sync);
}

int Pause(int);

static int volume=255,pan,pos_delta;
static __int64 samples_written;

bool hack_isplaying();

static int paused;

void setup_config(DS2config * cfg)
{
#ifdef HAVE_SSRC
	cfg->SetPCM(resample_freq,resample_bps,fmt_nch);
#else
	cfg->SetPCM(fmt_sr,fmt_bps,fmt_nch);
#endif
	cfg->SetCreatePrimary(cfg_createprimary);
	cfg->SetWindow(mod.hMainWindow);
	cfg->SetDeviceGUID(cfg_dev2);
	cfg->SetBuffer2(cfg_buf_ms,cfg_prebuf2);
	if (cfg_killsil) cfg->SetSilence((float)cfg_sil_db*(float)0.1);
}

int get_written_time();

static void do_ssrc_write(char * buf,int len);

int Open(int samplerate, int numchannels, int bitspersamp, int bufferlenms, int prebufferms)
{
	SYNC_IN;
	log_write("open");

	DS2::init(mod.hDllInstance);
	DS2 * wait=0;
	bool nofadein = pDS2 ? 1 : 0;
	bool nosetvol = nofadein;
	if (pDS2)
	{
		pDS2->SetCloseOnStop(0);
		if (pDS2->IsClosed())
		{
			pDS2->Release();pDS2=0;
			KILL_SSRC;
		}
		else
		{
			log_write("got ds2");
			if (fmt_nch!=(UINT)numchannels 
				//fixme: gapless transitions
//#ifndef HAVE_SSRC
				|| fmt_sr!=(UINT)samplerate || fmt_bps!=(UINT)bitspersamp 
//#endif
				|| cfg_fade_stop.get_time() || cfg_fade_start.get_time())
			{
#ifdef HAVE_SSRC
				use_finish=1;
				do_ssrc_write(0,0);
				KILL_SSRC;
#endif
				log_write("using wait");
				wait=pDS2;
				pDS2=0;
			}
		}
	}
	//need to do this BEFORE ds2_create, otherwise hack_isplaying it will always return 1
	UINT ft=hack_isplaying() ? cfg_fade_start.get_time() : cfg_fade_firststart.get_time();
	
	if (!pDS2)
	{
		nosetvol=0;
		log_write("doing new ds2 instance");
		fmt_sr=samplerate;
		fmt_nch=numchannels;
		fmt_bps=bitspersamp;

#ifdef HAVE_SSRC
		resample_freq=cfg_resample_freq;
		if (resample_freq<6000) resample_freq=6000;
		else if (resample_freq>192000) resample_freq=192000;
		resample_bps=bps_tab[cfg_resample_bps];

		if (fmt_sr!=resample_freq || fmt_bps!=resample_bps)
		{
			do_ssrc_create();
		}
#endif

		DS2config cfg;
		setup_config(&cfg);
		pDS2=DS2::create(&cfg);
		if (!pDS2)
		{
			log_write("bork bork");
			if (wait) wait->Release();
			const char * moo=cfg.GetError();
			if (moo) MessageBox(mod.hMainWindow,moo,NAME" error",MB_ICONERROR);
#ifdef DS2_CRT_HACKS
			cfg.clear();
#endif
			SYNC_OUT;
			return -1;
		}
#ifdef DS2_CRT_HACKS
		cfg.clear();
#endif
	}
	else
	{
#ifdef HAVE_SSRC
		if (pSSRC)
		{
			if (finished)
			{
				delete pSSRC;
				do_ssrc_create();
			}
			else use_finish=cfg_trackhack==0 ? 1 : 0;
		}
#endif
		pDS2->AlignData();
		pos_delta-=get_written_time();
	}
	
	if (!cfg_volume) volume=255;
	pDS2->SetPan(pan);
	if (ft && !nofadein)
	{
		log_write("fadein");
		pDS2->SetVolume(0);
		pDS2->Fade(ft,volume);
	}
	else if (!nosetvol) pDS2->SetVolume(volume);

	if (wait) pDS2->WaitFor(wait,0);

	pos_delta=0;
	samples_written=0;
	paused=0;
	log_write("done opening");
	wa2_hint=HINT_NONE;
	
	SYNC_OUT;
	return cfg_buf_ms;
}


void Close()
{
	log_write("close");
	SYNC_IN;
	if (pDS2)
	{
		log_write("got ds2");
		pDS2->KillEndGap();
		switch(wa2_hint)
		{
		case HINT_NONE:
			pDS2->FadeAndForget(cfg_fade_pause.get_time());
			pDS2=0;
			KILL_SSRC;
			break;
		case HINT_EOF:
			pDS2->FadeAndForget(cfg_fade_stop.get_time());
			pDS2=0;
			KILL_SSRC;
			break;
		case HINT_EOF_GAPLESS:
			pDS2->SetCloseOnStop(1);
			break;
		}
	}
	SYNC_OUT;
	log_write("done closing");
}

static void make_new_ds2()
{
#ifdef HAVE_SSRC
	KILL_SSRC;
	if (fmt_sr!=resample_freq || fmt_bps!=resample_bps)
	{
		do_ssrc_create();
	}
#endif

	DS2config cfg;
	setup_config(&cfg);
	pDS2=DS2::create(&cfg);

	if (pDS2)
	{	
		pDS2->SetPan(pan);
		pDS2->SetVolume(0);
		pDS2->Fade(fadetimehack,volume);
		fadetimehack=0;
	}
#ifdef DS2_CRT_HACKS
	cfg.clear();
#endif
}

#ifdef HAVE_SSRC
static UINT ssrc_extra_latency;

static void do_ssrc_write(char * buf,int len)
{
	if (!finished)
	{
		bool k=0;
		UINT nsiz;
		if (len>0) pSSRC->Write(buf,(UINT)len);
		else if (use_finish)
		{
			finished=1;
			pSSRC->Finish();
		}

		char * data=(char*)pSSRC->GetBuffer(&nsiz);
		UINT nsiz1=nsiz;
		while(nsiz)//ugly
		{
			UINT d=pDS2->CanWrite();
			if (d>nsiz) d=nsiz;
			if (d)
			{
				pDS2->WriteData(data,d,&k);
				data+=d;
				nsiz-=d;
				if (!nsiz) break;
			}
			ssrc_extra_latency=MulDiv(nsiz,1000,resample_freq*fmt_nch*(resample_bps>>3));
			SYNC_OUT;
			Sleep(1);//FIXME
			SYNC_IN;
		}
		pSSRC->Read(nsiz1);
		if (len==0) pDS2->ForcePlay();
	}
	ssrc_extra_latency=0;
}
#endif

int Write(char *buf, int len)
{
	SYNC_IN;
	wa2_hint=0;
#ifdef USE_LOG
	{
		char poo[256];
		wsprintf(poo,"write: %u bytes",len);
		log_write(poo);
	}
#endif
	if (paused)
	{
		SYNC_OUT;
		return 1;
	}
	bool k=0;
	if (!pDS2)
	{
		log_write("write: need new object");
		make_new_ds2();
		if (!pDS2 || !buf || !len)
		{
			SYNC_OUT;
			return 0;
		}
	}
	samples_written+=len/((fmt_bps>>3)*fmt_nch);
	int rv=0;
#ifdef HAVE_SSRC
	if (pSSRC)
	{
		do_ssrc_write(buf,len);
	}
	else
	{
		if (buf && len) rv=pDS2->WriteData(buf,len,&k);
		else pDS2->ForcePlay();
	}
#else
	if (buf && len) rv=pDS2->WriteData(buf,len,&k);
	else pDS2->ForcePlay();
#endif
	SYNC_OUT;
	return rv;
}

int CanWrite()
{
	int rv=0;
	SYNC_IN;
	if (!paused)
	{
		if (!pDS2) make_new_ds2();
		if (pDS2) rv=pDS2->CanWrite();
	}
	SYNC_OUT;
	return rv;
}

int IsPlaying()
{
	int rv=0;
	SYNC_IN;
	if (pDS2)
	{
		int foo=(int)cfg_fade_stop.get_time();
		pDS2->KillEndGap();
		int lat=pDS2->GetLatency();
		wa2_hint=HINT_EOF;
		if (foo>0)
		{
			rv=lat>foo;
		}
		else if (lat>(int)cfg_trackhack)
		{
			rv=1;
		}
		else
		{
			wa2_hint=HINT_EOF_GAPLESS;
			rv=0;
		}
	}
	SYNC_OUT;
	return rv;
}

int Pause(int new_state)
{
	SYNC_IN;
	int rv=paused;
	paused=new_state;
	if (new_state)
	{
		if (pDS2)
		{
			UINT ft=cfg_fade_pause.get_time();
			if (!ft)
			{
				pDS2->Pause(1);
			}
			else if (cfg_oldpause)
			{
				pDS2->FadeAndForget(ft);
				pDS2=0;
				KILL_SSRC;
				fadetimehack=ft;
			}
			else
			{
				pDS2->FadePause(ft);
			}
		}
	}
	else
	{
		if (pDS2) pDS2->Pause(0);
	}
	SYNC_OUT;
	return rv;
}

void SetVolume(int _volume) // volume is 0-255
{
	SYNC_IN;
	if (_volume!=-666 && cfg_volume)
	{
		volume=_volume;
		if (pDS2)
		{
			if (cfg_def_fade) pDS2->FadeX(150,_volume);
			else pDS2->SetVolume(_volume);
		}
	}
	SYNC_OUT;
}

void SetPan(int _pan) // pan is -128 to 128
{
	SYNC_IN;
	if (cfg_volume)
	{
		pan=_pan;
		if (pDS2) pDS2->SetPan(pan);
	}
	SYNC_OUT;
}

void Flush(int t)
{
	SYNC_IN;
	if (pDS2)
	{
		UINT t=cfg_fade_seek.get_time();
		if (t) 
		{
			pDS2->FadeAndForget(t);
			pDS2=0;
			KILL_SSRC;
			fadetimehack=t;
		}
		else pDS2->Flush();
	}
	samples_written=0;
	pos_delta=t;
	SYNC_OUT;
}

#if 1
_declspec(naked) int get_written_time()
{//little cute asm hack to avoid crt with 64bit ints
	_asm
	{
		xor eax,eax
		cmp dword ptr [pDS2],0
		je r
		//(samples_written*1000/sr)
		mov eax,1000
		mul dword ptr [samples_written]
		//eax:edx = low(samples_written)*1000
		push eax
		mov ecx,edx
		mov eax,1000
		mul dword ptr [samples_written+4]
		//eax:edx = hi(samples_written)*1000, we can ignore edx
		lea edx,[ecx+eax]
		pop eax
		div dword ptr [fmt_sr]
//		add eax,dword ptr [pos_delta]
r:
		ret
	}
}

#else
int get_written_time()//this ignores high 32bits of samples_written
{
	return MulDiv(samples_written,1000,sr);
}
#endif

static int GetWrittenTime()
{
	int rv;
	SYNC_IN;
	rv= pos_delta + get_written_time();
	SYNC_OUT;
	return rv;
}

int GetOutputTime()
{
	SYNC_IN;
	int rv=pos_delta+get_written_time();
	if (pDS2) rv-=pDS2->GetLatency();
#ifdef HAVE_SSRC
	if (pSSRC) rv-=ssrc_extra_latency ? ssrc_extra_latency : pSSRC->GetLatency();
#endif
	if (rv<0) rv=0;
	SYNC_OUT;
	return rv;
}


Out_Module mod=
{
	OUT_VER,
	NAME
#ifdef HAVE_SSRC
	" SSRC"
#endif
	,
	203968848,
	0,0,
	Config,
	About,

	Init,Quit,
	Open,
	
	Close,

	Write,

	CanWrite,
	
	IsPlaying,

	Pause,

	SetVolume,
	SetPan,

	Flush,

	GetOutputTime,
	GetWrittenTime,

};


BOOL APIENTRY DllMain(HANDLE hMod,DWORD r,void*)
{
	if (r==DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls((HMODULE)hMod);
		memset(&cfg_dev2,0,sizeof(cfg_dev2));
	}
    return TRUE;
}

extern "C"
{
	__declspec( dllexport ) Out_Module * winampGetOutModule()
	{
		return &mod;
	}
}


#ifdef DS2_CRT_HACKS

extern "C" {

void _cdecl _fltused() {}

_declspec(naked) long _cdecl _ftol()
{
	_asm
	{
		sub esp,2
		fstcw word ptr [esp]
		or word ptr [esp],0x0C00
		fldcw word ptr [esp]
		sub esp,6
		fistp qword ptr [esp]
		pop eax
		pop edx
		ret
	}
}


}


#endif