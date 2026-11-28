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

class au_base : public converter_base
{
private:
	int offset,cursor,len;
	int nch,srate;
	int encoding;

	int inited;
	int bps;

	MemBlock<char> data;

	int au_parse(MediaInfo* infos)
	{
		svc_fileReader * reader = infos->getReader();
		if (!reader) return 0;

		int maxlen = reader->getLength();
		
		reader->seek(0);
		DWORD temp=0;
		reader->read((char*)&temp,4);
		if (temp!=rev32('.snd')) return 0;
		DWORD data_offs;
		reader->read((char*)&data_offs,4);
		data_offs=rev32(data_offs);
		if (data_offs<24) return 0;
		DWORD data_size=0;
		reader->read((char*)&data_size,4);
		if (!data_size || data_size==0xFFFFFFFF) data_size = maxlen-data_offs;
		data_size=rev32(data_size);
		DWORD _encoding=0;
		reader->read((char*)&_encoding,4);
		encoding = rev32(_encoding);
		
		reader->read((char*)&temp,4);//sample rate
		srate=rev32(temp);
		
		reader->read((char*)&temp,4);//channels
		nch=rev32(temp);


		len=data_size;
		offset=data_offs;
		cursor=0;

		return 1;
	}

protected:
	virtual int au_get_bps(DWORD code)=0;//retrieves bps for compression code, or 0 if its not our code
	virtual void au_fix_order(void * data,int size,int bps,DWORD code) {}
	virtual const char * au_get_chunktype()=0;
	
	au_base() {inited=0;}
	~au_base() {}

public:
	

	virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype)
	{
		if (!name || chunktype || 
			(STRICMP(Std::extension(name),"AU") && STRICMP(Std::extension(name),"SND") ) 
 			|| !reader || !reader->canSeek()) return 0;
		__try
		{
			int maxlen=reader->getLength();
			if (maxlen<0x20) return 0;

			DWORD temp=0;
			reader->read((char*)&temp,4);
			if (temp!=rev32('.snd')) return 0;
			DWORD data_offs;
			reader->read((char*)&data_offs,4);
			data_offs=rev32(data_offs);
			if (data_offs<24) return 0;
			DWORD data_size=0;
			reader->read((char*)&data_size,4);
			if (!data_size) return 0;
			data_size=rev32(data_size);
			DWORD encoding=0;
			reader->read((char*)&encoding,4);
			
			reader->read((char*)&temp,4);//sample rate
			if (rev32(temp)<100 || rev32(temp)>1000000) return 0;
			reader->read((char*)&temp,4);//channels
			if (rev32(temp)==0 || rev32(temp)>100) return 0;

			return au_get_bps(rev32(encoding))!=0;

		}
		__finally
		{
			reader->seek(0);
		}
		return 0;
	}

	int init(MediaInfo * infos)
	{
		if (!inited)
		{
			if (!au_parse(infos)) return 0;

			bps=au_get_bps(encoding);

			if (!bps || !nch || !srate) return 0;

			len -= len % (nch * (bps/8));
			
			inited=1;
		}
		return 1;
	}
public:

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

		au_fix_order(data,delta,bps,encoding);

		ChunkInfosI *ci=new ChunkInfosI();
		ci->addInfo("srate",srate);
		ci->addInfo("bps",bps);
		ci->addInfo("nch",nch);

		chunk_list->setChunk(au_get_chunktype(),ptr,delta,ci);
		
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

	virtual const char *getConverterTo() { return au_get_chunktype(); }
};


class au_pcm : public au_base
{
protected:
	virtual int au_get_bps(DWORD code)
	{
		switch(code)
		{
		case 2:return 8;
		case 3:return 16;
		case 4:return 24;
		case 5:return 32;
		default:return 0;
		}
	}
	virtual const char * au_get_chunktype() {return "PCM";}
	virtual void au_fix_order(void * data,int size,int bps,DWORD code)
	{
		if (bps==8) fix_8bit_sign(data,size);
		else fix_byte_order(data,size,bps);
	}
public:
	static const char *getServiceName() { return "PCM AU/SND converter"; }
};

class au_ulaw : public au_base
{
protected:
	virtual int au_get_bps(DWORD code)
	{
		switch(code)
		{
		case 1:return 8;
		default:return 0;
		}
	}
	virtual const char * au_get_chunktype() {return "ULAW";}
	virtual void au_fix_order(void * data,int size,int bps,DWORD code) {fix_byte_order(data,size,bps);}
public:
	static const char *getServiceName() { return "u-Law AU/SND converter"; }
};

class au_alaw : public au_base
{
protected:
	virtual int au_get_bps(DWORD code)
	{
		switch(code)
		{
		case 0x1b:return 8;
		default:return 0;
		}
	}
	virtual const char * au_get_chunktype() {return "ALAW";}
public:
	static const char *getServiceName() { return "a-Law AU/SND converter"; }
};

class au_float : public au_base
{
protected:
	virtual int au_get_bps(DWORD code)
	{
		switch(code)
		{
		case 6:return 32;
		case 7:return 64;
		default:return 0;
		}
	}
	virtual const char * au_get_chunktype() {return "FLOAT";}
public:
	static const char *getServiceName() { return "Floatingpoint AU/SND converter"; }
};

converter_make_service(au_pcm);
converter_make_service(au_ulaw);
converter_make_service(au_alaw);
converter_make_service(au_float);

/*    0		magic number	the value 0x2e736e64 (ASCII ".snd")

    1		data offset	the offset, in octets, to the data part.
				The minimum valid number is 24 (decimal).

    2		data size	the size in octets, of the data part.
				If unknown, the value 0xffffffff should
				be used.

    3		encoding	the data encoding format:

				    value	format
				      1		8-bit ISDN u-law
				      2		8-bit linear PCM [REF-PCM]
				      3		16-bit linear PCM
				      4		24-bit linear PCM
				      5		32-bit linear PCM
				      6		32-bit IEEE floating point
				      7		64-bit IEEE floating point
				     23		8-bit ISDN u-law compressed
						using the CCITT G.721 ADPCM
						voice data encoding scheme.

    4		sample rate	the number of samples/second (e.g., 8000)

    5		channels	the number of interleaved channels (e.g., 1)
*/
