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
#ifndef _DS2_H
#define _DS2_H

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#define DS2_ENGINE_VER "v2.2.6"

#ifndef DS2_NO_DEVICES
#define DS2_HAVE_DEVICES
#endif

#ifndef DS2_NO_FADES
#define DS2_HAVE_FADES
#endif

class CriticalSection : public CRITICAL_SECTION
{
public:
	inline void Enter() {EnterCriticalSection(this);}
	inline void Leave() {LeaveCriticalSection(this);}
	CriticalSection() {InitializeCriticalSection(this);}
	~CriticalSection() {DeleteCriticalSection(this);}
	//BOOL TryEnter() {return TryEnterCriticalSection(this);}
};

typedef struct
{
	UINT sr,bps,nch;
	UINT buf_size_bytes,buf_size_ms;
	UINT pos_play,pos_write,latency,latency_ms;
	UINT lock_count;
	UINT underruns;
	UINT bytes_async;
	__int64 bytes_written,bytes_played;
	double vol_left,vol_right;
	GUID current_device;
	bool have_primary_buffer;
	bool paused;
	DWORD dscaps_flags;
	DWORD dscaps_flags_primary;
} DS2_REALTIME_STAT;

class DS2config	//config struct to pass to DS2::create(); if create messes up, this struct also returns error message
{
public:
	enum
	{
		DEFAULT_BUFFER=1500,
		DEFAULT_PREBUFFER=500,
	};
private:
	UINT sr,bps,nch;
	HWND wnd;
	bool create_primary;
	DWORD chan_mask;
	UINT ms;
	UINT preb;
	bool use_cpu_management;
	int volmode;
	int logvol_min;
	bool logfades;
	GUID guid;
	bool delayed_shutdown,prim_override;
	UINT _p_bps,_p_nch,_p_sr;
	float sil_db;
	UINT mixing;
	UINT refresh;
	UINT coop;
#ifdef DS2_HAVE_PITCH
	bool have_pitch;
#endif
	char error[256];
	inline void SetError(const char * n_error) {strncpy(error,n_error,255);}
	void SetErrorCodeMsg(const char * msg,DWORD code);
public:
	enum {
		MIXING_DEFAULT=0,
		MIXING_FORCE_HARDWARE=1,
		MIXING_FORCE_SOFTWARE=2
	};
	DS2config()
	{
		sr=44100;
		bps=16;
		nch=2;
		wnd=0;
		create_primary=0;
		error[0]=0;
		mixing=MIXING_DEFAULT;
		volmode=0;
		logfades=0;
//		logvol_min=100;
		chan_mask=0;
		ms=DEFAULT_BUFFER;
		preb=DEFAULT_PREBUFFER;
		memset(&guid,0,sizeof(guid));
		sil_db=0;
		delayed_shutdown=1;
		prim_override=0;
		_p_bps=_p_nch=_p_sr=0;
		use_cpu_management=0;
		refresh=10;
		coop=1;
#ifdef DS2_HAVE_PITCH
		have_pitch=0;
#endif
	}
	inline const char * GetError() {return error[0] ? error : 0;}
	void _inline SetPCM(UINT _sr,UINT _bps,UINT _nch) {sr=_sr;bps=_bps;nch=_nch;}
	void _inline SetWindow(HWND w) {wnd=w;}
	void _inline SetCreatePrimary(bool b) {create_primary=b;}
	void _inline SetChanMask(DWORD c) {chan_mask=c;}
	void _inline SetDeviceGUID(const GUID& g) {guid=g;}
	void _inline ResetDevice() {memset(&guid,0,sizeof(guid));}
	void _inline SetBuffer(UINT _ms=DEFAULT_BUFFER,UINT _preb=DEFAULT_PREBUFFER) {ms=_ms;preb=_preb;}
	void _inline SetSilence(float s) {sil_db=s;}// s is in negative dB
	void _inline SetDelayedShutdown(bool d) {delayed_shutdown=d;}//disable for AC97 shit, NOTE: this is global, not per-DS2 instance
	void _inline SetImmediateShutdown(bool d) {delayed_shutdown=!d;}
	void _inline SetPrimaryOverride(bool b) {prim_override=b;}
	void _inline SetPrimaryOverrideFormat(UINT _sr,UINT _bps,UINT _nch) {_p_bps=_bps;_p_nch=_nch;_p_sr=_sr;}
	void _inline SetVolMode(int mode,int logmin=100,bool _logfades=0) {volmode=mode;logvol_min=logmin;logfades=_logfades;}
	void _inline SetMixing(UINT m) {mixing=m;}
	void _inline SetCpuManagement(bool b) {use_cpu_management=b;}
	void _inline SetRefresh(UINT n) {refresh=n;}
	void _inline SetCoop(UINT n) {coop=n;}
#ifdef DS2_HAVE_PITCH
	void _inline SetHavePitch(bool b) {have_pitch=b;}
#endif
	friend class DS2;
};

class DS2
{
private:
	
	class SoundBlockList
	{
	private:
		class SoundBlock
		{
		private:
			void * data;
			UINT size,used,start;
		public:
			SoundBlock *next,*prev;
			inline SoundBlock() {data=0;size=0;next=0;prev=0;}
			inline ~SoundBlock() {if (data) free(data);}
			inline void SetData(void *new_data,UINT new_size)
			{
				if (!data) data=malloc(new_size);
				else if (new_size>size) data=realloc(data,size=new_size);
				memcpy(data,new_data,new_size);
				used=new_size;
				start=0;
			}
			inline void Advance(UINT d) {used-=d;start+=d;}
			inline const void * GetData() {return (char*)data+start;}
			inline UINT GetDataSize() {return used;}
			inline UINT Dump(void * out,UINT out_size)
			{
				const void * src=GetData();
				if (out_size>used) out_size=used;
				memcpy(out,src,out_size);
				Advance(out_size);
				return out_size;
			}
			inline void Clear() {used=0;}
		};
		void inline connect(SoundBlock * b1,SoundBlock * b2) {if (b1) b1->next=b2;if (b2) b2->prev=b1;}
		void inline advance(SoundBlock * &b) {b=b->next;}
		void inline goback(SoundBlock * &b) {b=b->prev;}
		void inline mount(SoundBlock * &first,SoundBlock * add)
		{
			connect(0,add);
			connect(add,first);
			first=add;
		}
		void mount2(SoundBlock * &first,SoundBlock * add,SoundBlock * &last)
		{
			mount(first,add);
			if (!last) last=add;			
		}
		SoundBlock * remove(SoundBlock * &list)
		{
			SoundBlock * b=list;
			if (b)
			{
				advance(list);
				connect(0,list);
				connect(b,0);
			}
			return b;
		}

		SoundBlock * removelast(SoundBlock * &first,SoundBlock * &last)
		{
			SoundBlock * b=last;
			if (b)
			{
				goback(last);
				connect(last,0);
				connect(0,b);
				if (!last) first=0;
			}
			return b;
		}
		
		UINT data_buffered;
		SoundBlock *b_first,*b_last,*b_free;
		int silence_filler;
	public:
		UINT DataSize() {return data_buffered;}
		SoundBlockList(int sil) {b_first=0;b_last=0;b_free=0;data_buffered=0;silence_filler=sil;}
		void AddBlock(void * data,UINT size);
		UINT DumpBlocks(void * out1,UINT size1);
		void Reset();
		~SoundBlockList();
	};

	DS2(DS2config * cfg);

	SoundBlockList BlockList;
	UINT LockCount;
	UINT Underruns;
	
	bool DoLock();

public:
	~DS2();  
	int WriteData(void * data,UINT size,bool *killswitch);//returns 1 on success and 0 on failure
	int WriteDataNow(void * data,UINT size);//sleep-less version, writes CanWrite() of bytes immediately, returns amount of data written
	int ForceWriteData(void * data,UINT size);//sleep-less force-write all the data w/o sleep/canwrite (async buffer has no size limit), use with caution

	/*
	how to use writedata shit

  a) just WriteData(), will sleep until its done
  b) WriteDataNow() until we're done (writes as much data as possible at the moment, without sleep)
  c) ForceWriteData() (evil!), then sleep while CanWrite()<0

    */

	void StartNewStream();

	UINT GetLatency();
	void _inline Release() {delete this;}//obsolete
	void Pause(int new_state);
	void SetVolume(double);
	inline void SetVolume_Int(int i) {SetVolume((double)i/255.0);}
	void SetPan(double);
	inline void SetPan_Int(int i) {SetPan((double)i/128.0);}
	void Flush();//causes problems with some um.. drivers
	int CanWrite();//can be negative !!!!!!! (eg. after ForceWriteData)
	inline UINT BufferStatusPercent() {return MulDiv(data_buffered+BlockList.DataSize(),buf_size,100);}
	void ForcePlay();
	void KillEndGap();

#ifdef DS2_HAVE_FADES
	void FadePause(UINT time);
	void FadeAndForget(UINT time);
	void Fade(UINT time,double destvol);
	inline void Fade_Int(UINT time,int destvol) {Fade(time,(double)destvol/255.0);}
	void FadeX(UINT time,double destvol);//actual fade time depends on volume difference
	inline void FadeX_Int(UINT time,int destvol) {FadeX(time,(double)destvol/255.0);}

#endif

	void WaitFor(DS2 * prev,UINT fadeout=0);

	//gapless mode stuff
	void SetCloseOnStop(bool b);
	bool IsClosed();
	
private:
#ifdef _DEBUG
	DWORD serial;
	UINT sync_n;
#endif

	int Open(DS2config * cfg);
	DS2 * next;	
	DS2 * wait;
	UINT prebuf;
	DWORD flags;
	enum
	{
		FLAG_UPDATED=1,
		FLAG_WAITED=1<<1,
		FLAG_NEED_PLAY_NOW=1<<2,
		FLAG_DIE_ON_STOP=1<<3,
		FLAG_CLOSE_ON_STOP=1<<4,
		FLAG_PAUSED=1<<5,
		FLAG_PLAYING=1<<6,
//		FLAG_UNDERRUNNING=1<<7,
		FLAG_USE_CPU_MNGMNT=1<<8,
		FLAG_FADEPAUSE=1<<9,
		FLAG_FADEPAUSING=1<<10,
		FLAG_STARTSIL=1<<11,
		FLAG_SWMIXED=1<<12,
	};
	IDirectSoundBuffer * pDSB;
	IDirectSound * myDS;
	UINT fmt_nch,fmt_bps,fmt_sr,fmt_mul;
	UINT buf_size,clear_size;
	__int64 last_nonsil,pos_delta,pos_delta2;
	UINT write_ahead;
	inline __int64 GetCurPos() {return data_written-data_buffered;}
	__int64 GetSafeWrite();
	__int64 data_written;
	UINT data_buffered;
	UINT silence_buffered;

	UINT bytes2ms(UINT bytes);
	UINT ms2bytes(UINT ms);
	

#ifdef DS2_HAVE_FADES
	class DsVolCtrl
	{
	private:
		bool IsFading;
		int VolMode;
		double LogVolMin;

		double FadeSrcVol,FadeDstVol,FadeSrcPan,FadeDstPan;
		__int64 FadeSrcTime,FadeDstTime;

		__int64 CurTime;

		double CurVol,CurPan,LastVol,LastPan;
		double DestVolHack,DestPanHack;
		bool LogFades;
		void MapVol(double Vol,double Pan,double &NewVol,double &NewPan);
	public:
		DsVolCtrl(int VolMode,double LogVolMin,bool logfades);
		void SetFade(__int64 duration,double destvol,double destpan);
		inline void SetFadeVol(__int64 duration,double destvol) {SetFade(duration,destvol,DestPanHack);}
		inline void SetFadePan(__int64 duration,double destpan) {SetFade(duration,DestVolHack,destpan);}
		__int64 RelFade(__int64 max,double destvol);
		void SetTime(__int64 time);
		void SetVolume(double vol);
		void SetPan(double pan);
		void Apply(IDirectSoundBuffer * pDSB);
//		inline double GetCurVol() {return CurVol;}
		inline double GetDestVol() {return DestVolHack;}
		inline void Reset() {CurTime=0;FadeDstTime=-1;}
		double Stat_GetVolLeft();
		double Stat_GetVolRight();

		bool Fading();
	};
	DsVolCtrl VolCtrl;
	double fadepause_orgvol;
	UINT fadepause_time;
	UINT waitfade;

#else
	class DsVolCtrl
	{
	private:
		double CurVol,CurPan;
	public:
		DsVolCtrl() {CurVol=1;CurPan=0;}
		inline void SetTime(__int64) {}
		inline void SetFade(__int64,double) {}
		inline void SetFadeVol(__int64,double,double) {}
		inline void SetFadePan(__int64,double,double) {}
		inline void SetVolume(double v) {CurVol=v;}
		inline void SetPan(double p) {CurPan=p;}
//		inline __int64 RelFade(__int64 max,double destvol) {return 0;}
		void Apply(IDirectSoundBuffer * pDSB);
		inline bool Fading() {return 0;}
//		inline double GetCurVol() {return CurVol;}
//		inline double GetDestVol() {return CurVol;}
		inline void Reset() {}
	};
	DsVolCtrl VolCtrl;
#endif

	void SetVolumeI(double);
	void SetPanI(int _pan);
	void _setvol();
	void _setpan();
	void update_pos();
	void ds_stop();
	void ds_kill();
	bool Update();

	void reset_vars();
	void do_reset_vars();

	int silence_delta;
	void test_silence(char * buf,int len,int * first,int* last);

	static DWORD WINAPI ThreadFunc(void*);
	UINT _inline _align() {return (fmt_bps>>3)*fmt_nch;}
	UINT _inline _align_var(UINT var) {return var-(var%_align());}


	static void SYNC_IN();
	static void SYNC_OUT();

	typedef HRESULT (WINAPI *tDirectSoundCreate)( const GUID * lpGuid,  LPDIRECTSOUND * ppDS,    IUnknown FAR * pUnkOuter  );
	static tDirectSoundCreate pDirectSoundCreate;

	static IDirectSound * pDS;

public:

	static void Init();//init is OBSOLETE
	static void Quit(bool wait=0);//must be called on exit, NOT from DllMain, its ok to call it if init never happened

	static DS2 * Create(DS2config * cfg);

	static bool InitDLL();//used internally

	static HRESULT myDirectSoundCreate(const GUID * g,IDirectSound ** out);

	static void SetTotalTime(__int64);
	static __int64 GetTotalTime();
	static UINT InstanceCount();

	void GetRealtimeStat(DS2_REALTIME_STAT * stat);
	static bool GetRealtimeStatStatic(DS2_REALTIME_STAT * stat);
	__int64 GetOutputTime();

#ifdef DS2_HAVE_DEVICES
	static bool TryGetDevCaps(const GUID *g,LPDSCAPS pCaps,DWORD * speakercfg=0);//for current device

	typedef HRESULT (WINAPI *tDirectSoundEnumerate)(LPDSENUMCALLBACK lpDSEnumCallback,LPVOID lpContext);
	static tDirectSoundEnumerate pDirectSoundEnumerate;

	static GUID GetCurDev();
#endif


#ifdef DS2_HAVE_PITCH
	void SetPitch(double p);
#endif

};


#ifdef DS2_HAVE_DEVICES
class DsDevEnum
{
private:
	typedef struct tagDEV
	{
		tagDEV * next;
		char * name;
		GUID guid;
	} DS_DEV;

	DS_DEV * pDev;

	DS_DEV * ds_devs;
	static BOOL WINAPI DSEnumCallback(LPGUID guid,LPCTSTR desc,LPCTSTR,DS_DEV *** var);
public:
	
	const GUID GetGuid();
	inline const char * GetName(const char * def="device not found") {return pDev ? pDev->name : def;}
	inline bool operator++(int) {if (pDev) pDev=pDev->next; return pDev?true:false;}
	inline operator bool() {return pDev?true:false;}
	bool FindGuid(const GUID & g);
	bool FindDefault();
	bool FindName(const char * n);

	DsDevEnum();
	~DsDevEnum();
	inline Reset() {pDev=ds_devs;}

	static bool GetCapsFromGuid(const GUID *dev,LPDSCAPS pCaps,DWORD * speakercfg=0);

	inline bool GetCaps(LPDSCAPS pCaps,DWORD * speakercfg=0) {return GetCapsFromGuid(&pDev->guid,pCaps,speakercfg);}

//	const GUID GuidFromName(const char* name);
//	const char * NameFromGuid(const GUID & g, char * def="device not found");
};

//helpers
class DsDevEnumGuid : public DsDevEnum
{
public:
	DsDevEnumGuid(const GUID & g) {FindGuid(g);}
};

class DsDevEnumName : public DsDevEnum
{
public:
	DsDevEnumName(const char * n) {FindName(n);}
};

class DsDevEnumDefault : public DsDevEnum
{
public:
	DsDevEnumDefault() {FindDefault();}
};

#endif

#endif