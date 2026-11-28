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

void fix_byte_order(void * data,int size,int bps)
{
	if (bps<=8 || bps>128) return;
	bps = (bps+7)/8;
	int n;
	size/=bps;
	BYTE * ptr=(BYTE*)data;
	char temp[16];
	for(n=0;n<size;n++)
	{
		int b;
		for(b=0;b<bps;b++) temp[b]=ptr[bps-1-b];
		memcpy(ptr,temp,bps);
		ptr+=bps;
	}
}

void fix_8bit_sign(void * data,int size)
{
	BYTE * ptr=(BYTE*)data;
	int n;
	for(n=0;n<size;n++) ptr[n]^=0x80;
}

#pragma pack(push)
#pragma pack(1)

struct AIFF_COMM
{
	WORD Channels;
	DWORD SampleFrames;
	WORD SampleSize;
	BYTE SampleRate[10];
	DWORD CompressionType;
	int _SampleRate()
	{
		char blah[10];
		int n;
		for(n=0;n<10;n++) {blah[n]=SampleRate[9-n];}
		_asm
		{
			fld tbyte ptr [blah]
			fistp n
		}
		return n;
	}
};

#pragma pack(pop)

static DWORD iff_seek(svc_fileReader * reader,DWORD code,UINT &pos,UINT max)
{//seeks IFF chunks to <code> block, returns size
	DWORD dw,s;
	while(1)
	{
		pos+=8;
		if (pos>=max) return 0;
		if (reader->read((char*)&dw,4)!=4) return 0;
		if (reader->read((char*)&s,4)!=4) return 0;
		s=rev32(s);
		if (((signed)s<0)) return 0;
		if (dw==code) return s;
		if (pos+s>max) return 0;
		reader->seek(pos+=s+(s&1));
	}
}

class aiff_base : public converter_base
{
protected:
	int offset,cursor,len;
	AIFF_COMM * comm;
	UINT comm_size;
	int aiff_parse(MediaInfo* infos)
	{
		svc_fileReader * reader = infos->getReader();
		if (!reader) return 0;

		int maxlen = reader->getLength();
		
		UINT pos=12;
		reader->seek(pos);
		comm_size=iff_seek(reader,rev32('COMM'),pos,len);
		if (!comm_size) return 0;
		comm = (AIFF_COMM*)malloc(comm_size);
		if (!comm) return 0;
		reader->read((char*)comm,comm_size);
		pos += comm_size + (comm_size&1);
		
		len = iff_seek(reader,rev32('SSND'),pos,maxlen);
		
		if (len>(int)(maxlen-pos)) len=(int)(maxlen-pos);
		
		if (len<=0) return 0;

		offset=pos;
		cursor=0;

		return 1;
	}

	virtual int aiff_isourtype(DWORD code)=0;

	aiff_base() {comm=0;comm_size=0;}
	~aiff_base() {if (comm) free(comm);}

public:
	

	virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype)
	{
		if (!name || chunktype || 
			(STRICMP(Std::extension(name),"AIF") && STRICMP(Std::extension(name),"AIFF") && STRICMP(Std::extension(name),"IFF")) 
			|| !reader || !reader->canSeek()) return 0;
		__try
		{
			int maxlen=reader->getLength();
			if (maxlen<0x20) return 0;

			DWORD temp;
			reader->read((char*)&temp,4);
			if (temp!=rev32('FORM')) return 0;
			reader->read((char*)&temp,4);
			//ignore possibly wrong size so we can play broken files
			reader->read((char*)&temp,4);
			if (temp==rev32('AIFF')) return aiff_isourtype(rev32('NONE'));
			else if (temp!=rev32('AIFC')) return 0;
			
			reader->read((char*)&temp,4);

			UINT pos=12;
			DWORD comm_size=iff_seek(reader,rev32('COMM'),pos,maxlen);

			if (comm_size < sizeof(AIFF_COMM)) return 0;

			AIFF_COMM comm;

			if (reader->read((char*)&comm,sizeof(comm)) != sizeof(comm)) return 0;
			
			return aiff_isourtype(comm.CompressionType);
		}
		__finally
		{
			reader->seek(0);
		}
		return 0;
	}
};


class aiff_pcm : public aiff_base
{
private:
	int inited;
	int nch,bps,srate;

	MemBlock<char> data;

	int init(MediaInfo * infos)
	{
		if (!inited)
		{
			if (!aiff_parse(infos)) return 0;

			nch = rev16(comm->Channels);
			bps = rev16(comm->SampleSize);
			srate = comm->_SampleRate();

			len -= len % (nch * (bps/8));
			
			inited=1;
		}
		return 1;
	}
	int aiff_isourtype(DWORD c) {return c==rev32('NONE') || c==rev32('none');}
public:
	aiff_pcm() {inited=0;}
	static const char *getServiceName() { return "PCM AIFF converter"; }
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

		if (bps==16 || bps==32) fix_byte_order(data,delta,bps);
		else if (bps==8) fix_8bit_sign(data,delta);

		ChunkInfosI *ci=new ChunkInfosI();
		ci->addInfo("srate",srate);
		ci->addInfo("bps",bps);
		ci->addInfo("nch",nch);

		chunk_list->setChunk("PCM",ptr,delta,ci);
		
		cursor += delta;

		return cursor < len;

	}
	virtual int getInfos(MediaInfo *infos)
	{
		if (!init(infos)) return 0;
		int bytespersec = (bps/8)*nch*srate;
		infos->setInfo(StringPrintInfo(bytespersec,nch,srate));
		infos->setLength(MulDiv(len,1000,bytespersec));
		infos->setTitle(Std::filename(infos->getFilename()));
		return 1;
	}
};


//TODO: IMA ADPCM decoder for AIFF

/*

class aiff_ima : public aiff_base
{
private:
	ima_adpcm_decoder ima_dec;
	int inited;
	int nch,srate;

	MemBlock<char> data;
	MemBlock<short> data_decoded;

	int init(MediaInfo * infos)
	{
		if (!inited)
		{
			if (!aiff_parse(infos)) return 0;

			nch = rev16(comm->Channels);
			if (rev16(comm->SampleSize)!=4) return 0;
			srate = comm->_SampleRate();

			//TODO align length
			
			ima_dec.reset(0,0);

			inited=1;
		}
		return 1;
	}
	int aiff_isourtype(DWORD c) {return c==rev32('ima4') || c==rev32('IMA4');}
public:
	aiff_ima() {inited=0;}
	static const char *getServiceName() { return "IMA ADPCM AIFF converter"; }
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

				union
				{
					short sh;
					BYTE b[2];
				} u;
				for(c=0;c<fmt.nch;c++)
				{
					if (eof || *ks) break;
					u.b[1]=read_byte(ks);
					u.b[0]=read_byte(ks);
					csamp[c]=(int)u.sh&0xFFFFFF80;
					idx[c]=u.sh&0x7F;
					UINT n;
					for(n=0;n<64;n++)
					{
						sampbuf[n][c]=csamp[c]=get_sample(csamp[c],&idx[c],ks);
					}
					if (eof || *ks) break;
				}
				sbuf_pos=0;


		ChunkInfosI *ci=new ChunkInfosI();
		ci->addInfo("srate",srate);
		ci->addInfo("bps",bps);
		ci->addInfo("nch",nch);

		chunk_list->setChunk("PCM",ptr,delta,ci);
		
		cursor += delta;

		return cursor < len;

	}
	virtual int getInfos(MediaInfo *infos)
	{
		if (!init(infos)) return 0;
		int bytespersec = (bps/8)*nch*srate;
		infos->setInfo(StringPrintInfo(bytespersec,nch,srate));
		infos->setLength(MulDiv(len,1000,bytespersec));
		infos->setTitle(Std::filename(infos->getFilename()));
		return 1;
	}
};
*/

converter_make_service(aiff_pcm);
