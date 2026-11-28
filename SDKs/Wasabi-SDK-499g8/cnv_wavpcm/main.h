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
#if defined(WIN32) && !defined(NOWIN32HACK)
#define WAV_WIN32
#endif


#ifdef WAV_WIN32
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#else

#define EXTRA_DECODERS

#include "../common/platform/platform.h"

#pragma pack(push,1)
//blah
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
				    /* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, NEAR *NPWAVEFORMATEX, FAR *LPWAVEFORMATEX;

#pragma pack(pop)

#define WAVE_FORMAT_PCM     1
#define  WAVE_FORMAT_DVI_ADPCM  0x0011  /*  Intel Corporation  */
#define  WAVE_FORMAT_IMA_ADPCM  (WAVE_FORMAT_DVI_ADPCM) /*  Intel Corporation  */
#define  WAVE_FORMAT_ALAW       0x0006  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_MULAW      0x0007  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_ADPCM      0x0002  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_IEEE_FLOAT 0x0003  /*  Microsoft Corporation  */

#endif

#include "../studio/services/svc_mediaconverter.h"
#include "../studio/services/servicei.h"
#include "../studio/corecb.h"
#include "../studio/wac.h"
#include "../attribs/cfgitemi.h"
#include "../attribs/attrint.h"

//old tempura hack
//todo: convert this to svc_fileReader
class WReader 
{
  protected:
	WReader() { }
  public:
	virtual int Open(char *url, bool *killswitch)=0; 
	virtual int Read(char *buffer, UINT length, bool *killswitch)=0; 
	virtual int GetLength(void)=0; 
	virtual int CanSeek(void)=0;
	virtual int Seek(UINT position, bool *killswitch)=0;
	virtual void Release()=0;
};



#define log_write(x) //todo

void get_temp_file(char* fn);

#ifndef USE_PP_CRT
#define uitoa(x,y) sprintf(y,"%u",x)
#define atoui atoi
#define div64(x,y) ((x)/(y))
void * z_malloc(UINT x);
void * newhack(void *,UINT s);
#define _new(x) ( (x*)newhack(new x,sizeof(x)) )
#else
#define _new(x) (new x)
#endif

typedef struct tagFORMATSPEC
{
	UINT sr,nch,bps,chan;
	tagFORMATSPEC() {sr=nch=bps=chan=0;}
} FORMATSPEC;

#define MAX_WAVE_DESC 512

typedef struct
{
	FORMATSPEC fmt;
	UINT kbps;
	UINT len;
	char desc[MAX_WAVE_DESC];
} WAVEINFO;

class CWavPlayer
{
public:
	virtual void Release()=0;
	virtual UINT Read(void* buf,UINT buf_size,bool* ks)=0;
	virtual UINT GetLengthSamp()=0;
	virtual void SeekSamp(UINT,UINT*,bool* ks)=0;
	virtual void GetInfo(WAVEINFO*);
	virtual BOOL CanSeek()=0;
	virtual int processData(MediaInfo * i,ChunkList* cl,char * tempbuf,UINT tempsize,bool * ks);
	virtual BOOL useswa3shiz() {return 0;}//1 if has its own processdata implementation
	UINT GetLengthMS();
	void SeekMS(UINT,UINT*,bool* ks);
	FORMATSPEC fmt;
};

class CWavPlayerRd : public CWavPlayer
{
public:
	virtual void Release();
	virtual BOOL CanSeek();
	WReader* r;
};

#define RAW_INV_ORDER 1
#define RAW_FIX_8BIT 2

typedef struct
{
	UINT start,length,filler,filler_s;
} CHUNK;

#define LOOP_MAX 0x20

class CChunkList
{
public:
	void add(UINT start,UINT length);
	void add_filler(UINT length,UINT f,UINT l);
	void start_loop(UINT times);//if we know loop count on start, pass it here then pass 0 to finish_loop()
	void finish_loop(UINT times);
	void rel();
	CHUNK* data;

	UINT loop_starts[LOOP_MAX];//stack-like
	UINT loop_counts[LOOP_MAX];
	UINT loop_ptr;
	
	UINT data_ptr,data_size;
};

CChunkList* clist_create();

class CLoader
{
public:
	WReader* r;
	bool* ks;
	bool info;
	char * error;
	CWavPlayer* raw_create(FORMATSPEC *fmt,DWORD flags);
	CWavPlayer* wav_create();
	CWavPlayer* aiff_create();
	CWavPlayer* _8svx_create(DWORD sz,BOOL);
	CWavPlayer* au_create();
	CWavPlayer* voc_create();
	CWavPlayer* voc_finish(UINT comp,FORMATSPEC *fmt);
	CWavPlayer* cvt16_create(CWavPlayer* p);
	CWavPlayer* mono_create(CWavPlayer* p);
#ifdef WAV_WIN32
	CWavPlayer* acm_create(WAVEFORMATEX* src_fmt);
#endif
	CWavPlayer* delta_create(FORMATSPEC* fmt);
	CWavPlayer* aiff_ima_create(FORMATSPEC* fmt);
	CWavPlayer* aud_create();
	CWavPlayer* aud_create_ws();
	CWavPlayer* g72x_create(FORMATSPEC* fmt,UINT bits);
	CWavPlayer* acm_cvt_create(CWavPlayer* p,FORMATSPEC* dst);
	CWavPlayer* float_create(CWavPlayer* p);
#ifdef EXTRA_DECODERS
	CWavPlayer* mulaw_create(UINT nch,UINT sr,bool inv);
	CWavPlayer* alaw_create(UINT nch,UINT sr,bool inv);
	CWavPlayer* ima_wav_create(UINT nch,UINT sr,UINT block);
#else
	CWavPlayer * foo_create(UINT nch,UINT sr,UINT tag);
//acmhacks
	CWavPlayer* CLoader::mulaw_create(UINT nch,UINT sr,bool inv)
	{
		return foo_create(nch,sr,WAVE_FORMAT_MULAW);
	}

	CWavPlayer* CLoader::alaw_create(UINT nch,UINT sr,bool inv)
	{
		return foo_create(nch,sr,WAVE_FORMAT_ALAW);
	}
#endif
	CWavPlayer* mp3_create(UINT framesize,WAVEFORMATEX *);

	WReader* partialize(UINT start,UINT size);
	CWavPlayer* run(int type);
	int do_chunks(CChunkList* l);

	int make_sure_we_can_seek();
	int cache();
};

#ifdef WAV_WIN32
void build_wfx(FORMATSPEC *fmt,WAVEFORMATEX *wfx);
#endif
UINT samp2byte(UINT s,FORMATSPEC* fmt);
UINT byte2samp(UINT b,FORMATSPEC* fmt);
UINT byte2ms(UINT b,FORMATSPEC* fmt);
void print_fmt(char* buf,FORMATSPEC* fmt,char* tag);
void do_info(WAVEINFO* wi,FORMATSPEC* fmt,char* tag,UINT len);

#define _rv(X) ((((DWORD)(X)&0xFF)<<24)|(((DWORD)(X)&0xFF00)<<8)|(((DWORD)(X)&0xFF0000)>>8)|(((DWORD)(X)&0xFF000000)>>24))
DWORD _inline rev32(DWORD x) {return _rv(x);}
WORD _inline rev16(WORD x) {return (x>>8)|(x<<8);}

#ifdef LLAMA
#define N_EXTS 10
#else
#define N_EXTS 15
#endif

extern char* exts[N_EXTS];
void build_fmtstring();

//extern char cfg_extra_exts[256];
extern DWORD cfg_ext_mask;
//extern char fmts_string[1024];
extern bool cfg_vocsil,cfg_downcvt,cfg_mono,cfg_exploop;

extern UINT cfg_fdest;

struct CWriteBuf
{
public:
	BYTE* buf;
	UINT ptr,sz;

	void* finish(UINT* s);
	void* finish_t(UINT* s);// w/o realloc
	void freebuf();
	BOOL write(void* p,UINT s);
	UINT get_offs() {return ptr;}
	void setptr(UINT p) {ptr=p;}
	BOOL xfer(WReader* r,UINT s,bool*);
};

CWriteBuf* wb_create();

WReader* hread_create(void* mem,UINT sz);
WReader* partialize(WReader*,UINT start,UINT size);

WReader* seekabilize(WReader* r);
WReader* cache(WReader* r);

extern UINT priority_tab[7];





class CWrapReader : public WReader 
{
  public:
	virtual int Open(char *url, bool *killswitch) {return 0;}; 
	virtual int Read(char *buffer, UINT length, bool *killswitch); 
	virtual int GetLength(void); 
	virtual int CanSeek(void);
	virtual int Seek(UINT position, bool *killswitch);
	virtual void Release() {};
	svc_fileReader * r;
	CWrapReader() {r=0;}
};


class Wave : public svc_mediaConverterI {
public:
  Wave();
  virtual ~Wave();

  // service
  static const char *getServiceName() { return "Waveform Decoder"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype);
  virtual const char *getConverterTo() { return "PCM"; }

  virtual int getInfos(MediaInfo *infos);

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void) { return 0; }

  // callbacks
  virtual int corecb_onSeeked(int newpos);
private:

  CWrapReader m_rd;

  int needseek;
  char samplebuffer[4096];

  char * fn;
  CWavPlayer * plr;
  int open(const char * url,bool * ks);
};

extern char ERROR_FILE_DAMAGED[];

#pragma warning(disable:4309)
#pragma warning(disable:4244)
#pragma warning(disable:4305)