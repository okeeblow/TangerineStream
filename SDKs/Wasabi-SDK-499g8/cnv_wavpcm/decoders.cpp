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

static const char ima_indextab[ 8 ] = { -1, -1, -1, -1, 2, 4, 6, 8};

static const short ima_steptab[ 89 ] = {
7, 8, 9, 10, 11, 12, 13, 14,
16, 17, 19, 21, 23, 25, 28, 31,
34, 37, 41, 45, 50, 55, 60, 66,
73, 80, 88, 97, 107, 118, 130, 143,
157, 173, 190, 209, 230, 253, 279, 307,
337, 371, 408, 449, 494, 544, 598, 658,
724, 796, 876, 963, 1060, 1166, 1282, 1411,
1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
32767 };


int ima_adpcm_decoder::decode(int c)
{
	int delta=ima_steptab[index]>>3;
	if (c & 4) delta+=ima_steptab[index];
	if (c & 2) delta+=ima_steptab[index]>>1;
	if (c & 1) delta+=ima_steptab[index]>>2;
	if (c & 8) sample-=delta; else sample+=delta;
	if (sample>32767) sample=32767;
	else if (sample<-32768) sample=-32768;
	index+=ima_indextab[c&7];
	if (index<0) index=0;
	else if (index>88) index=88;
	return sample;
}


#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */


static BYTE inv_bits(BYTE b)
{
	BYTE ret=0;
	UINT n;
	for(n=0;n<8;n++)
	{
		ret<<=1;
		ret|=b&1;
		b>>=1;
	}
	return ret;
}

#define	BIAS		(0x84)		/* Bias for linear code. */


class NOVTABLE ulaw_alaw_base : public svc_mediaConverterI
{
private:
	MemBlock<short> data;
protected:
	virtual int do_sample(BYTE b)=0;
	virtual const char * get_name()=0;
public:
	virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype)
	{
		return chunktype && !STRICMP(chunktype,get_name());
	}

	virtual const char *getConverterTo() {return "PCM";}

	virtual int getInfos(MediaInfo *infos) {return 0; }

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
	{
		Chunk * c=chunk_list->getChunk(get_name());
		if (!c) return 1;
		int bytes=c->getSize();
		BYTE *src=(BYTE*)c->getData();
		short * out=data.setSize(bytes);
		int n;
		for(n=0;n<bytes;n++) out[n]=do_sample(src[n]);


		ChunkInfosI *ci=new ChunkInfosI();

		ci->addInfo("srate",c->getInfo("srate"));
		ci->addInfo("bps",16);
		ci->addInfo("nch",c->getInfo("nch"));

		chunk_list->setChunk("PCM",out,2*bytes,ci);

		return 1;
	}
};

class ulaw_converter : public ulaw_alaw_base
{
protected:
	const char * get_name() {return "ULAW";}
	int do_sample(BYTE u_val)
	{
		int		t;

		u_val = ~u_val;

		t = ((u_val & QUANT_MASK) << 3) + BIAS;
		t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

		return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
	}
public:
	static const char *getServiceName() { return "u-Law decoder"; }
};

class alaw_converter : public ulaw_alaw_base
{
protected:
	const char * get_name() {return "ALAW";}
	int do_sample(BYTE a_val)
	{
		int		t;
		int		seg;

		a_val ^= 0x55;

		t = (a_val & QUANT_MASK) << 4;
		seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
		switch (seg) {
		case 0:
			t += 8;
			break;
		case 1:
			t += 0x108;
			break;
		default:
			t += 0x108;
			t <<= seg - 1;
		}
		return ((a_val & SIGN_BIT) ? t : -t);
	}
public:
	static const char *getServiceName() { return "A-Law decoder"; }
};

converter_make_service(ulaw_converter);
converter_make_service(alaw_converter);
