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
#include "wave.h"
#undef _new
#include "../bfc/svc_enum.h"

//hack to use wa3 shiz to decode mp3 wavs



//  int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) { return _call(PROCESSDATA,0,infos,chunk_list,killswitch); }


class CMP3Player : public CWavPlayerRd, public SvcEnumT<svc_mediaConverter>
{
public:
//	FORMATSPEC fmt;
//	WReader* r;
	svc_mediaConverter * cvt;
	UINT fs;
	UINT kbps;
	char * frame;
	UINT poo_size;
/*
typedef struct
{
	FORMATSPEC fmt;
	UINT kbps;
	UINT len;
	char desc[MAX_WAVE_DESC];
} WAVEINFO;
*/
	virtual void GetInfo(WAVEINFO* i)
	{
		i->fmt=fmt;
		i->kbps=kbps;
		i->len=GetLengthMS();
		wsprintf(i->desc,"%ukbps %s %ukhz",kbps,fmt.nch==1 ? "mono" : "stereo", fmt.sr/1000);
	}
	virtual BOOL useswa3shiz() {return 1;}

	CMP3Player(UINT framesize,WReader * _r,WAVEFORMATEX * wfx)
	{
		fmt.nch=wfx->nChannels;
		fmt.sr=wfx->nSamplesPerSec;
		fmt.bps=16;
		kbps=wfx->nAvgBytesPerSec;
		kbps=kbps/125+(kbps%125)/63;

		if (MulDiv(framesize,fmt.sr,wfx->nAvgBytesPerSec)>800) poo_size=1152;
		else poo_size=576;
		
		frame=0;
		r=0;
		cvt=getFirst();
		if (!cvt) return;
		fs=framesize;
		frame=(char*)malloc(fs);
		r=_r;
	}


	virtual void Release()
	{
		if (frame) free(frame);
		if (cvt) release(cvt);
		if (r) r->Release();
		delete this;
	}

	virtual UINT Read(void* buf,UINT bytes,bool* ks)
	{
		return 0;
/*		UINT written=0;
		char * bptr=(char*)buf;
		while(written<bytes)
		{
			UINT d=buf_bytes;
			if (!d)
			{
				doframe();
				d=buf_bytes;
				if (!d) break;
			}
			if (d>bytes-written) d=bytes-written;

			if (bptr) 
				memcpy(bptr+written,buffer,d);

			buffer+=d;
			buf_bytes-=d;
			written+=d;
		}
		return written;*/
	}
	
	virtual int processData(MediaInfo *i,ChunkList* cl,char * tempbuf,UINT tempsize,bool * ks)
	{
		bool kfoo=0;
		UINT s=(UINT)r->Read(frame,fs,&kfoo);
		int rv=0;
		if (s==fs)
		{
			cl->setChunk("MP3",frame,fs,0);
			rv=cvt->processData(i,cl,ks);
			cl->delChunk("MP3");
			if (rv)
			{
				Chunk * c=cl->getChunk("PCM");
				if (c)
				{

					fmt.sr=c->getInfo("srate");
					fmt.nch=c->getInfo("nch");
					fmt.bps=c->getInfo("bps");
				}
			}
		}
		return rv;
	}

	virtual UINT GetLengthSamp()
	{
		return ((r->GetLength()/fs)-1)*poo_size;
	}

	virtual void SeekSamp(UINT s,UINT* ps,bool* ks)
	{
		if (ps) *ps=s;
		r->Seek((s/poo_size)*fs,ks);
		Read(0,(s%poo_size)*(fmt.bps>>3)*fmt.nch,ks);
	}
	
protected:
  virtual int testService(svc_mediaConverter *svc) {
    return svc->canConvertFrom(0,0,"MP3") && !STRICMP(svc->getConverterTo(),"PCM");
  }
};

CWavPlayer * CLoader::mp3_create(UINT framesize,WAVEFORMATEX * wfx)
{
	if (r->GetLength()<(signed)(framesize<<1)) return 0;//blah
	CMP3Player * p=new CMP3Player(framesize,r,wfx);
	if (!p->cvt)
	{
		p->Release();
		p=0;
	}
	return p;
}