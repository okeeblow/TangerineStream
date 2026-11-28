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
#include "main.h"
#include "adpcm.h"

class CImaPlayer : public CAdpcmPlayer
{
public:
	int get_sample(int last,int *idx,bool* ks);
	virtual void GetInfo(WAVEINFO* wi) {GetInfoInternal(wi,"IMA ADPCM");}
};

static const char IndexTab[ 8 ] = { -1, -1, -1, -1, 2, 4, 6, 8};

static const short StepTab[ 89 ] = {
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

int CImaPlayer::get_sample(int sample,int *pidx,bool* ks)
{
	UINT c=read_nibble(ks);

	if (eof) return 0;

	int _idx=*pidx;
	int delta=StepTab[_idx]>>3;
	if (c & 4) delta+=StepTab[_idx];
	if (c & 2) delta+=StepTab[_idx]>>1;
	if (c & 1) delta+=StepTab[_idx]>>2;
	if (c & 8) sample-=delta; else sample+=delta;
	if (sample>32767) sample=32767;
	else if (sample<-32768) sample=-32768;
	_idx+=IndexTab[c&7];
	if (_idx<0) _idx=0;
	else if (_idx>88) _idx=88;
	*pidx=_idx;
	return sample;
}

//raw IMA version, not used at the moment
/*
UINT CImaPlayer::Read(void* buf,UINT buf_size,bool* ks)
{
	short* out=(short*)buf;
	UINT wrt=0;
	UINT n;
	while(wrt<buf_size && !eof && !*ks)
	{
		for(n=0;n<fmt.nch;n++)
		{
			*(out++)=csamp[n]=get_sample(csamp[n],&idx[n],ks);
			if (*ks || eof) break;
			wrt+=2;
		}
	}
	return wrt;
}
*/

class CImaPlayer_apple : public CImaPlayer
{
public:
	int csamp[4];
	int idx[4];

	short sampbuf[64][4];
	UINT sbuf_pos;
	virtual UINT Read(void* buf,UINT buf_size,bool* ks)
	{
		UINT wr=0;
		UINT c;
		short* out=(short*)buf;
		while(wr<buf_size)
		{
			if (sbuf_pos>=64)
			{
				if (eof) break;
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
			}
			for(c=0;c<fmt.nch;c++)
			{
				*(out++)=sampbuf[sbuf_pos][c];
				wr+=2;
			}
			sbuf_pos++;
		}
		return wr;
	}
	virtual UINT GetLengthSamp()
	{
		UINT b=r->GetLength()/34;
		b/=fmt.nch;
		return b<<6;		//*64
	}
	virtual void SeekSamp(UINT p,UINT* b,bool* ks)
	{
		if (b) *b=samp2byte(p&~63,&fmt);
		sbuf_pos=-1;
		flush((p>>6)*34*fmt.nch,ks);
	}
	virtual BOOL CanSeek() {return 1;}
};

CWavPlayer* CLoader::aiff_ima_create(FORMATSPEC* fmt)
{
	if (fmt->nch>4) return 0;
	CImaPlayer_apple* p=_new(CImaPlayer_apple);
	if (!p) return 0;
	p->bits=4;
	p->sbuf_pos=-1;
	p->r=r;
	p->fmt=*fmt;
	p->fmt.bps=16;
	return p;
}

#include "aud.h"

class CImaPlayer_aud : public CImaPlayer
{
public:
	int csamp;
	int idx;

	UINT chunk_size,decoded,outsize,dec_pos;
	virtual UINT Read(void* buf,UINT buf_size,bool* ks)
	{
		UINT wr=0;
		short* out=(short*)buf;
		while(wr<buf_size)
		{
			if (!chunk_size || decoded>=chunk_size)
			{
				if (eof) break;
				chunk_header chd;
				if (read_buf(&chd,sizeof(chd),ks)!=sizeof(chd)) break;
				if (chd.ID!=0xDEAF) return 0;//invalid file
				chunk_size=chd.Size<<1;
				decoded=0;
			}
			csamp=get_sample(csamp,&idx,ks);
			if (out) *(out++)=csamp;
			if (eof || *ks) break;
			decoded++;
			dec_pos++;
			wr+=2;
		}
		return wr;
	}
	virtual UINT GetLengthSamp() {return outsize>>1;}
	virtual void SeekSamp(UINT p,UINT* b,bool* ks)
	{
		if (b) *b=samp2byte(p,&fmt);
		if (p<dec_pos)
		{
			decoded=0;
			chunk_size=0;
			dec_pos=0;
			flush(0,ks);
			Read(0,p<<1,ks);
		}
		else
		{
			Read(0,(p-dec_pos)<<1,ks);
		}
	}
	virtual BOOL CanSeek() {return 1;}
};


CWavPlayer* CLoader::aud_create()
{
	//detect old aud format
	chunk_header chd;
	{
		AUD_header1 ahd;
		if (r->Read((char*)&ahd,sizeof(ahd),ks)!=sizeof(ahd)) return 0;
		if (r->Read((char*)&chd,sizeof(chd),ks)!=sizeof(ahd)) return 0;
		r->Seek(0,ks);
		if (*ks) return 0;
		if (ahd.SamplesPerSec==22050 && ahd.fmt==FMT_WS && chd.ID==0xDEAF)
		{
			return aud_create_ws();
		}
	}
	AUD_header ahd;
	
	if (r->Read((char*)&ahd,sizeof(ahd),ks)!=sizeof(ahd)) return 0;
	if (ahd.SamplesPerSec!=22050 || ahd.fmt!=FMT_IMA) return 0;
	if (r->Read((char*)&chd,sizeof(chd),ks)!=sizeof(chd)) return 0;
	if (chd.ID!=0xDEAF) return 0;
	if (*ks) return 0;
	r->Seek(sizeof(ahd),ks);
	if (*ks) return 0;
	CImaPlayer_aud *p=_new(CImaPlayer_aud);
	if (!p) return 0;
	if (!partialize(sizeof(ahd),ahd.Size)) return 0;
	p->r=r;
	p->fmt.sr=22050;
	p->fmt.nch=1;
	p->fmt.bps=16;
	p->bits=4;
	p->outsize=ahd.OutSize;
	return p;
}

#ifdef EXTRA_DECODERS
//IMA ADPCM WAVs on non-windoze

//me likes ripped Red Alert music

class CImaPlayer_wav : public CImaPlayer
{
public:
	int *idx,*csamp,*sbuf;
	UINT todo,outsize,dec_pos;
	UINT block,cc,blocksamples;
	int blah;
	
	virtual UINT Read(void* buf,UINT buf_size,bool* ks)
	{
		UINT wr=0;
		short* out=(short*)buf;
		while(wr<buf_size)
		{
			if (!todo)
			{
				if (eof) break;
				UINT n;
				for(n=0;n<fmt.nch;n++)
				{
					csamp[n]=(short)read_bits(16,ks);
					if (buf) *(out++)=csamp[n];
					idx[n]=read_bits(8,ks);
					read_bits(8,ks);
					wr+=2;
				}
				todo=(blocksamples-1)*fmt.nch;
				cc=0;
				blah=8;
			}
			if (eof || *ks) break;

			if (blah==8)
			{
				UINT n,c;
				for(c=0;c<fmt.nch;c++)
				{
					for(n=0;n<8;n++)
					{
						sbuf[c*8+n]=csamp[c]=get_sample(csamp[c],&idx[c],ks);
					}					
				}
				blah=0;
			}
			//bill sucks
			if (buf) *(out++)=sbuf[cc*8+blah];

			if (++cc==fmt.nch) {blah++;cc=0;}
			wr+=2;
			todo--;
		}
		return wr;
	}
	virtual UINT GetLengthSamp()
	{
		UINT l=r->GetLength();
		if (l!=-1)
		{
			l=l/block*blocksamples;
		}
		return l;
	}
	virtual void SeekSamp(UINT p,UINT* b,bool* ks)
	{
		if (b) *b=samp2byte(p,&fmt);
		cc=0;
		blah=0;
		todo=0;
		flush( p/blocksamples*block ,ks);
		Read(0,2*fmt.nch*(p%blocksamples),ks);
	}
	virtual BOOL CanSeek() {return 1;}
	virtual void Release()
	{
		if (csamp) free(csamp);
		if (idx) free(idx);
		CImaPlayer::Release();
	}
};

CWavPlayer* CLoader::ima_wav_create(UINT nch,UINT sr,UINT block)
{
	CImaPlayer_wav *p=_new(CImaPlayer_wav);
	if (p)
	{
		p->r=r;
		p->fmt.sr=sr;
		p->fmt.nch=nch;
		p->fmt.bps=16;
		p->bits=4;
		p->block=block;
		p->blocksamples=(block/nch-4)*2+1;
		p->idx=(int*)malloc(sizeof(int)*nch);
		p->csamp=(int*)malloc(sizeof(int)*nch);
		p->sbuf=(int*)malloc(sizeof(int*)*nch*8);
		p->cc=0;
		p->todo=0;
		p->blah=0;
	}
	return p;
}


#endif