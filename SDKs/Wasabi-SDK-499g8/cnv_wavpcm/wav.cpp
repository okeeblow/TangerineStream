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
#include "cnv_wavpcm.h"

#include <mmreg.h>
#include <msacm.h>


class wav_base : public converter_base
{
protected:
	virtual bool wav_isourtype(UINT type)=0;
	int offset,cursor,len;
	WAVEFORMATEX * wfx;
	int wfx_size;

	int wav_parse(MediaInfo * infos)
	{
		svc_fileReader * reader=infos->getReader();
		if (!reader) return 0;
		
		long temp;

		reader->seek(8+8);
		reader->read((char*)&temp,4);//waveformatex size
		
		
		if (temp<sizeof(PCMWAVEFORMAT)) return 0;
		
		wfx_size=temp;
		wfx=(WAVEFORMATEX*)malloc(wfx_size);

		reader->read((char*)wfx,temp);

	
		int position = 12;

		DWORD code;
		int filesize = reader->getLength();


		do {
			position += 8 + temp + (temp&1);//word-align all blocksizes
			reader->seek(position);
			if (reader->read((char*)&code,4)!=4) return 0;
			if (reader->read((char*)&temp,4)!=4) return 0;

		} while(code!=rev32('data'));
		
		offset=position+8;
		cursor=0;
		len=temp;
		if (len + offset > filesize) len = filesize - offset;
		len -= len % wfx->nBlockAlign;

		return len>0;

	}

	void wav_do_infos(MediaInfo * infos)
	{
		infos->setLength(MulDiv(len,1000,wfx->nAvgBytesPerSec));

		infos->setTitle(Std::filename(infos->getFilename()));

		infos->setInfo(StringPrintInfo(wfx->nAvgBytesPerSec,wfx->nChannels,wfx->nSamplesPerSec));

	}

	wav_base() {wfx=0;}
	~wav_base() {if (wfx) free(wfx);}
	
public:

	virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype)
	{
		if (!name || chunktype || STRICMP(Std::extension(name),"WAV") || !reader || !reader->canSeek()) return 0;

		int len=reader->getLength();
		if (len<8 + 4 + 8 + 14 + 8) return 0;
		__try
		{
			
			DWORD dw;
			reader->read((char*)&dw,4);
			if (dw!=rev32('RIFF')) return 0;
			reader->read((char*)&dw,4);
			//if (dw>len-8) return 0;//possible incomplete files
			reader->read((char*)&dw,4);
			if (dw!=rev32('WAVE')) return 0;
			reader->read((char*)&dw,4);
			if (dw!=rev32('fmt ')) return 0;//fixme fmt doesnt have to be first
			reader->read((char*)&dw,4);
			if (dw<14) return 0;//fixme fmt doesnt have to be first
			dw=0;
			reader->read((char*)&dw,2);
			return wav_isourtype(dw);
		} __finally
		{
			reader->seek(0);
		}
		return 0;
	}
  

};


class wav_nodec_base : public wav_base
{
private:
	int inited;
	int nch,bps,srate;

	MemBlock<char> data;

	int init(MediaInfo * infos)
	{
		if (inited) return 1;

		wav_parse(infos);

		nch=wfx->nChannels;
		srate=wfx->nSamplesPerSec;

		bps = wav_check_bps(wfx->wBitsPerSample);

		if (!bps || !nch || !srate) return 0;

		len -= len % ((bps/8) * nch);

		inited=1;
		return 1;
	}

protected:
	virtual const char * wav_get_chunktype()=0;
	virtual int wav_check_bps(int)=0;
public:	
	wav_nodec_base() {inited=0;}

  
	virtual int getInfos(MediaInfo *infos)
	{
		if (!init(infos)) return 0;

		wav_do_infos(infos);

		return 1;
	}

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
	{
		if (!init(infos)) return 0;
		
		svc_fileReader * reader = infos->getReader();

		if (need_seek >= 0)
		{
			cursor = nch * (bps/8) * MulDiv(need_seek,srate,1000);

			if (cursor>len) cursor=len;

			reader->seek(offset + cursor);

			need_seek=-1;
		}

		int delta = 576 * (bps/8) * nch;
		if (delta > len - cursor) delta = len - cursor;
		
		char * ptr = data.setSize(delta);
		infos->getReader()->read(ptr,delta);

		ChunkInfosI *ci=new ChunkInfosI();
		ci->addInfo("srate",srate);
		ci->addInfo("bps",bps);
		ci->addInfo("nch",nch);

		chunk_list->setChunk(wav_get_chunktype(),ptr,delta,ci);
		
		cursor += delta;

		return cursor < len;
	}

	virtual const char *getConverterTo() { return wav_get_chunktype(); }
};

class wav_pcm : public wav_nodec_base
{
protected:
	virtual bool wav_isourtype(UINT type) {return type==WAVE_FORMAT_PCM;}
	virtual const char * wav_get_chunktype() {return "PCM";}
	virtual int wav_check_bps(int bps) {return (bps+7)&~7;}
public:
	static const char *getServiceName() { return "PCM WAV converter"; }
};

class wav_float : public wav_nodec_base
{
protected:
	virtual bool wav_isourtype(UINT type) {return type==WAVE_FORMAT_PCM;}
	virtual const char * wav_get_chunktype() {return "PCM";}
	virtual int wav_check_bps(int bps) {return (bps == 8 || bps == 16 || bps==32 || bps==64) ? bps : 0;}
public:
	static const char *getServiceName() { return "Floatingpoint WAV converter"; }
};

class wav_ulaw : public wav_nodec_base
{
protected:
	virtual bool wav_isourtype(UINT type) {return type==WAVE_FORMAT_MULAW;}
	virtual const char * wav_get_chunktype() {return "ULAW";}
	virtual int wav_check_bps(int bps) {return bps==8 ? 8 : 0;}
public:
	static const char *getServiceName() { return "u-Law WAV converter"; }
};

class wav_alaw : public wav_nodec_base
{
protected:
	virtual bool wav_isourtype(UINT type) {return type==WAVE_FORMAT_ALAW;}
	virtual const char * wav_get_chunktype() {return "ALAW";}
	virtual int wav_check_bps(int bps) {return bps==8 ? 8 : 0;}
public:
	static const char *getServiceName() { return "A-Law WAV converter"; }
};

class acm_converter//from in_wave
{
private:
	enum
	{
		INBUF=0x20000
	};
	HACMSTREAM hacm;
	BYTE* inbuf,*outbuf;
	UINT inbuf_s,outbuf_s,inbuf_b,outbuf_b,outbuf_start;
	ACMSTREAMHEADER ahd;
	bool initialized;
protected:
	virtual UINT acm_read_callback(void * buf,UINT size,bool * killswitch)=0;

	void acm_cleanup()
	{
		if (hacm)
		{
			if (ahd.fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
			{
				ahd.cbSrcLength=inbuf_s;
				acmStreamUnprepareHeader(hacm,&ahd,0);
			}
			acmStreamClose(hacm,0);
			hacm=0;
		}
		if (inbuf) {free(inbuf);inbuf=0;}
		if (outbuf) {free(outbuf);outbuf=0;}
	}

	acm_converter()
	{
		hacm=0;
		inbuf=outbuf=0;
		inbuf_s=outbuf_s=inbuf_b=outbuf_b=outbuf_start=0;
		memset(&ahd,0,sizeof(ahd));
		initialized=0;
	}
	~acm_converter() {acm_cleanup();}

	UINT acm_read(void* buf,UINT buf_size,bool* ks)
	{
		if (*ks) return 0;
		if (outbuf_b>=buf_size)
		{
			if (buf) memcpy(buf,outbuf+outbuf_start,buf_size);
			outbuf_b-=buf_size;
			outbuf_start+=buf_size;
			return buf_size;
		}
		UINT ret_d=0;
		if (outbuf_b)
		{
			if (buf)
			{
				memcpy(buf,outbuf+outbuf_start,outbuf_b);
				buf=(BYTE*)buf+outbuf_b;
			}
			buf_size-=outbuf_b;
			ret_d=outbuf_b;
			outbuf_b=0;
		}
		outbuf_start=0;
		int rd=inbuf_s-inbuf_b;
		rd=acm_read_callback(inbuf+inbuf_b,rd,ks);
		if (*ks) return ret_d;
		if (rd>0)
		{
			inbuf_b+=rd;
		}
		ahd.cbSrcLength=inbuf_b;
		DWORD flagz=ACM_STREAMCONVERTF_BLOCKALIGN;
		if (rd<=0) flagz|=ACM_STREAMCONVERTF_END;
		if (!initialized)
		{
			initialized=1;
			flagz|=ACM_STREAMCONVERTF_START;
		}
		acmStreamConvert(hacm,&ahd,flagz);
		inbuf_b-=ahd.cbSrcLengthUsed;
		if (inbuf_b)
		{
			memcpy(inbuf,inbuf+ahd.cbSrcLengthUsed,inbuf_b);
		}
		outbuf_b=ahd.cbDstLengthUsed;
		outbuf_start=0;
		if (rd<=0 && buf_size>outbuf_b) buf_size=outbuf_b;
		return ret_d+acm_read(buf,buf_size,ks);
	}

	int acm_init(WAVEFORMATEX* src,WAVEFORMATEX* dst)
	{
		if (acmStreamOpen(&hacm,0,src,dst,0,0,0,0)) goto fail;

		inbuf_s=INBUF - INBUF%src->nBlockAlign;
		outbuf_s = MulDiv(inbuf_s,dst->nAvgBytesPerSec,(src->wFormatTag==2) ? (src->nAvgBytesPerSec>>2) : src->nAvgBytesPerSec);//wFormatTag==2 => gayass ms adpcm driver, borked avgbytespersec
		if (!inbuf_s || !outbuf_s) goto fail;
		inbuf=(BYTE*)malloc(inbuf_s);
		if (!inbuf) goto fail;
		outbuf=(BYTE*)malloc(outbuf_s);
		if (!outbuf) goto fail;
		ahd.cbStruct=sizeof(ahd);
		ahd.pbSrc=inbuf;
		ahd.cbSrcLength=inbuf_s;
		ahd.pbDst=outbuf;
		ahd.cbDstLength=outbuf_s;
		if (acmStreamPrepareHeader(hacm,&ahd,0)) goto fail;
		return 1;
	fail:
		acm_cleanup();
		return 0;
	};

	void acm_flush()
	{
		outbuf_b=inbuf_b=outbuf_start=0;initialized=0;
	}
};

class wav_acm : public wav_base, private acm_converter
{
private:
	int nch,bps,srate;

	int infos_inited,decode_inited;

	svc_fileReader * reader;

	MemBlock<char> data;

	virtual bool wav_isourtype(UINT type)
	{
		return 
			type!=WAVE_FORMAT_PCM && type!=WAVE_FORMAT_MPEGLAYER3 && type!=WAVE_FORMAT_IEEE_FLOAT
			&& type!=WAVE_FORMAT_ALAW && type!=WAVE_FORMAT_MULAW;

	}

	int infos_init(MediaInfo * infos)
	{
		if (infos_inited) return 1;
		if (!wav_parse(infos)) return 0;
		infos_inited=1;
		return 1;
	}

	int decode_init(MediaInfo * infos)
	{
		if (decode_inited) return 1;
		if (!infos_init(infos)) return 0;
		WAVEFORMATEX wfx_dest;
		memset(&wfx_dest,0,sizeof(wfx_dest));
		wfx_dest.wFormatTag=WAVE_FORMAT_PCM;
		if (acmFormatSuggest(0,wfx,&wfx_dest,sizeof(wfx_dest),ACM_FORMATSUGGESTF_WFORMATTAG))
		{//no codec
			return 0;
		}

		if (!acm_init(wfx,&wfx_dest)) return 0;

		nch=wfx_dest.nChannels;
		bps=wfx_dest.wBitsPerSample;
		srate=wfx_dest.nSamplesPerSec;


		decode_inited=1;
		return 1;
	}

	virtual UINT acm_read_callback(void * buf,UINT size,bool * killswitch)
	{
		int delta=(int)size;
		if (delta>len-cursor) delta=len-cursor;
		reader->read((char*)buf,delta);
		cursor+=delta;
		return (UINT)delta;
	}

public:
	wav_acm() {infos_inited=0;decode_inited=0;}

	static const char *getServiceName() { return "ACM WAV converter"; }
  
	virtual int getInfos(MediaInfo *infos)
	{
		if (!infos_init(infos)) return 0;

		wav_do_infos(infos);

		return 1;
	}

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
	{
		if (!decode_init(infos)) return 0;

		reader = infos->getReader();
		if (!reader) return 0;

		if (need_seek>=0)
		{
			cursor = MulDiv(need_seek,wfx->nAvgBytesPerSec,1000);
			cursor -= cursor % wfx->nBlockAlign;
			
			if (cursor>len) cursor=len;

			reader->seek(offset + cursor);

			acm_flush();
			
			need_seek=-1;
		}

		int delta = 576 * (bps/8) * nch;

		char * ptr = data.setSize(delta);

		delta = acm_read(ptr,delta,killswitch);

		if (delta>0)
		{

			ChunkInfosI *ci=new ChunkInfosI();
			ci->addInfo("srate",srate);
			ci->addInfo("bps",bps);
			ci->addInfo("nch",nch);

			chunk_list->setChunk("PCM",ptr,delta,ci);
			
			return 1;
		}
		else return 0;
	}

};

class wav_mp3 : public wav_base
{
private:
	virtual bool wav_isourtype(UINT type) {return type==WAVE_FORMAT_MPEGLAYER3;}
	int inited;
	int blocksize;
	MemBlock<char> data;
	int init(MediaInfo * infos)
	{
		if (inited) return 1;
		if (!wav_parse(infos)) return 0;
		if (wfx_size<sizeof(WAVEFORMATEX)+8) return 0;
		blocksize=*(WORD*)((char*)wfx+sizeof(WAVEFORMATEX)+6);
		if (blocksize<=0) return 0;
		inited=1;
		return 1;
	}
public:
	wav_mp3() {inited=0;}
	~wav_mp3() {}

	static const char *getServiceName() { return "MPEG layer 3 WAV converter"; }
  
	virtual const char *getConverterTo() { return "MP3"; }

	virtual int getInfos(MediaInfo *infos)
	{
		if (!init(infos)) return 0;

		wav_do_infos(infos);

		return 1;
	}

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
	{
		if (!init(infos)) return 0;

		svc_fileReader * reader = infos->getReader();

		if (need_seek>=0)
		{
			cursor = MulDiv(need_seek,wfx->nAvgBytesPerSec,1000);
			cursor -= cursor % blocksize;

			reader->seek(offset + cursor);

			need_seek=-1;
		}

		int delta = blocksize;
		if (delta > len - cursor) delta = len - cursor;
		
		char * ptr = data.setSize(delta);
		reader->read(ptr,delta);

		cursor+=delta;

		chunk_list->setChunk("MP3",ptr,delta,0);
		
		return cursor<len;
	}

};

converter_make_service(wav_pcm);
converter_make_service(wav_float);
converter_make_service(wav_alaw);
converter_make_service(wav_ulaw);
converter_make_service(wav_acm);
converter_make_service(wav_mp3);