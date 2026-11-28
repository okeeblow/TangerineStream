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

#ifdef WAV_WIN32
void build_wfx(FORMATSPEC *fmt,WAVEFORMATEX *wfx)
{
	wfx->wFormatTag=WAVE_FORMAT_PCM;
	wfx->nChannels=fmt->nch;
	wfx->nSamplesPerSec=fmt->sr;
	wfx->nAvgBytesPerSec=fmt->nch*(fmt->bps>>3)*fmt->sr;
	wfx->nBlockAlign=fmt->nch*(fmt->bps>>3);
	wfx->wBitsPerSample=fmt->bps;
	wfx->cbSize=0;
}

#endif

void print_fmt(char* buf,FORMATSPEC* fmt,char* tag)
{
	if (!tag) tag="PCM";
	char * _c;
	char ch[16];
	if (fmt->nch==1) _c="Mono";
	else if (fmt->nch==2) _c="Stereo";
	else {sprintf(ch,"%u channels",fmt->nch);_c=ch;}
	sprintf(buf,"%s, %u.%ukHz, %s, %u-bit",tag,fmt->sr/1000,(fmt->sr/100)%10,_c,fmt->bps);
}

void do_info(WAVEINFO* wi,FORMATSPEC* fmt,char* tag,UINT len)
{
	wi->fmt=*fmt;
	wi->len=len;
	UINT bps=fmt->bps*fmt->sr*fmt->nch;
	wi->kbps=bps/1000;
	if ((bps%1000)>500) wi->kbps++;
	print_fmt(wi->desc,fmt,tag);
}

void CWavPlayer::GetInfo(WAVEINFO* wi)
{
	do_info(wi,&fmt,0,GetLengthMS());
}


BOOL CWavPlayerRd::CanSeek() {return r->CanSeek();}

UINT byte2ms(UINT b,FORMATSPEC* fmt)
{
	return MulDiv(byte2samp(b,fmt),1000,fmt->sr);
}

UINT samp2byte(UINT s,FORMATSPEC* fmt)
{
	return s*fmt->nch*(fmt->bps>>3);
}

UINT byte2samp(UINT b,FORMATSPEC* fmt)
{
	return b/(fmt->nch*(fmt->bps>>3));
}

UINT CWavPlayer::GetLengthMS()
{
	UINT v=GetLengthSamp();
	if (v!=-1) v=MulDiv(v,1000,fmt.sr);
	return v;
}

void CWavPlayer::SeekMS(UINT p,UINT* b,bool* ks)
{
	SeekSamp(MulDiv(p,fmt.sr,1000),b,ks);
}

class CRawPlayer : public CWavPlayerRd
{
public:
	DWORD flags;
	virtual UINT Read(void* buf,UINT buf_size,bool*);
	virtual UINT GetLengthSamp();
	virtual void SeekSamp(UINT p,UINT* b,bool* ks)
	{
		UINT pos=samp2byte(p,&fmt);
		if (b) *b=pos;
		r->Seek(pos,ks);
	}

};

void CWavPlayerRd::Release() {if (r) r->Release();delete this;}

UINT CRawPlayer::GetLengthSamp()
{
	return byte2samp(r->GetLength(),&fmt);
}

UINT CRawPlayer::Read(void* buf,UINT buf_size,bool* ks)
{
	UINT v=r->Read((char*)buf,buf_size,ks);
	if (v>0)
	{
		if (flags&RAW_FIX_8BIT)
		{
			if (fmt.bps==8)
			{
				UINT n;
				BYTE* b=(BYTE*)buf;
				for(n=0;n<v;n++)
				{
					b[n]^=0x80;
				}
			}
		}
		if (flags&RAW_INV_ORDER)
		{
			UINT bc=(fmt.bps&0xFFFF)>>3;
			UINT bc1=bc>>1;
			BYTE* b=(BYTE*)buf;
			UINT n,n1;
			for(n=0;n<v;n+=bc)
			{
				for(n1=0;n1<bc1;n1++)
				{
					BYTE t=b[n1];
					b[n1]=b[bc-1-n1];
					b[bc-1-n1]=t;
				}
				b+=bc;
			}
		}
	}
	return v;
}

CWavPlayer* CLoader::raw_create(FORMATSPEC* fmt,DWORD flags)
{
	CRawPlayer* p=_new(CRawPlayer);
	if (p)
	{
		p->fmt=*fmt;
		p->r=r;
		p->flags=flags;
	}
	return p;
}


void* CWriteBuf::finish(UINT* s)
{
	void* r=0;
	if (buf)
	{
		r=realloc(buf,ptr);
		*s=ptr;
	}
	delete this;
	return r;
}

void* CWriteBuf::finish_t(UINT* s)
{
	void* r=0;
	if (buf)
	{
		r=buf;
		*s=ptr;
	}
	delete this;
	return r;
}

void CWriteBuf::freebuf()
{
	if (buf) free(buf);
	delete this;
}

BOOL CWriteBuf::write(void* p,UINT s)
{
	UINT np=ptr+s;
	if (!buf) goto q;
	if (np>sz)
	{
		do {
			sz<<=1;
		} while(np>sz);
		void * nb=realloc(buf,sz);
		if (nb) buf=(BYTE*)nb;
		else {free(buf);buf=0;goto q;}
	}
	if (p) memcpy(buf+ptr,p,s);
q:
	ptr=np;
	return buf ? 1 : 0;
}

CWriteBuf* wb_create()
{
	CWriteBuf* r=_new(CWriteBuf);
	if (r)
	{
		r->sz=0x1000;
		r->ptr=0;
		if (!(r->buf=(BYTE*)z_malloc(r->sz)))
		{
			delete r;
			r=0;
		}
	}
	return r;
}

BOOL CWriteBuf::xfer(WReader* r,UINT s,bool *ks)
{
	UINT np=ptr+s;
	if (!buf) goto q;
	if (np>sz)
	{
		do {
			sz<<=1;
		} while(np>sz);
		void * nb=realloc(buf,sz);
		if (nb) buf=(BYTE*)nb;
		else {free(buf);buf=0;goto q;}
	}
	if (r) r->Read((char*)(buf+ptr),s,ks);
q:
	ptr=np;
	return buf ? 1 : 0;
}

class CConverter : public CWavPlayer
{
public:
	CWavPlayer* plr;
	virtual void Release() {plr->Release();delete this;}
	virtual UINT GetLengthSamp() {return plr->GetLengthSamp();}
	virtual void GetInfo(WAVEINFO* i) {plr->GetInfo(i);}
	virtual BOOL CanSeek() {return plr->CanSeek();}
};

#define TBUF_S 0x1200

class C16: public CConverter
{
public:
	BYTE tbuf[TBUF_S];
	UINT sb;
	virtual UINT Read(void* buf,UINT buf_size,bool *ks);
	virtual void SeekSamp(UINT p,UINT* b,bool* ks);
};

UINT C16::Read(void* buf,UINT buf_size,bool *ks)
{
	switch(sb)
	{
	case 8:
		{
			UINT nsam=buf_size>>1;
			UINT r=0;
			short* out=(short*)buf;
			while(nsam)
			{
				UINT d=nsam;
				if (d>TBUF_S) d=TBUF_S;
				d=plr->Read(tbuf,d,ks);
				if (!d) break;
				nsam-=d;
				r+=d<<1;
				BYTE* s=tbuf;
				do {
					UINT b=*(s++);
					*(out++)=(b|(b<<8))^0x8000;
					d--;
				} while(d);
			}
			return r;
		}
	case 16:
		return plr->Read(buf,buf_size,ks);
	case 24:
	case 32:
	case 48:
	case 56:
	case 64:
		{
			UINT mul=sb>>3;
			UINT nsam=buf_size>>1;
			UINT r=0;
			short* out=(short*)buf;
			while(nsam)
			{
				UINT d=nsam*mul;
				if (d>TBUF_S) d=TBUF_S;
				d=plr->Read(tbuf,d,ks);
				if (!d) break;
				d/=mul;
				nsam-=d;
				r+=d<<1;
				BYTE* s=tbuf+(mul-2);
				do {
					*(out++)=*(short*)s;
					s+=mul;
					d--;
				} while(d);
			}
			return r;
		}
	default:
		return 0;
	}
}

void C16::SeekSamp(UINT p,UINT* b,bool* ks)
{
	plr->SeekSamp(p,b,ks);
	if (b) *b=MulDiv(*b,16,sb);
}

CWavPlayer* CLoader::cvt16_create(CWavPlayer* p)
{
	C16* c=_new(C16);
	if (c)
	{
		c->plr=p;
		c->fmt=p->fmt;
		c->fmt.bps=16;
		c->sb=p->fmt.bps;
	}
	return c;
}

class CMono : public CConverter
{
public:
	BYTE tbuf[TBUF_S+8];
	UINT sb;
	virtual UINT Read(void* buf,UINT buf_size,bool *ks);
	virtual void SeekSamp(UINT p,UINT* b,bool* ks);
};

UINT CMono::Read(void* buf,UINT buf_size,bool *ks)
{
	UINT wr=0;
	BYTE* out=(BYTE*)buf;
	while(buf_size && !*ks)
	{
		UINT rd=buf_size*plr->fmt.nch;
		if (rd>TBUF_S) rd=TBUF_S;
		rd=plr->Read(tbuf,rd,ks);
		if (*ks || !rd) break;
		rd/=plr->fmt.nch;
		wr+=rd;
		buf_size-=rd;
		rd/=sb;
		BYTE* p=tbuf;
		UINT n,c;
		UINT nc=plr->fmt.nch;
		__int64 rmask=(1<<fmt.bps)-1;
		__int64 msb=(sb>1) ? (1<<(fmt.bps-1)) : 0;
		__int64 smask=~rmask;
		__int64 min,max;
		if (sb>1)
		{
			min=smask|msb;
			max=msb-1;
		}
		else min=0;max=rmask;

		for(n=0;n<rd;n++)
		{
			__int64 val=0;
			for(c=0;c<nc;c++)
			{
				__int64 cv=*(__int64*)p&rmask;
				p+=sb;
				if (cv&msb) cv|=smask;
				val+=cv;
			}
			div64(val,nc);
			if (val>max) val=max;
			else if (val<min) val=min;
			
			BYTE* vp=(BYTE*)&val;
			for(c=0;c<sb;c++)
			{
				*(out++)=vp[c];
			}
		}
	}
	return wr;
}

void CMono::SeekSamp(UINT p,UINT* b,bool* ks)
{
	plr->SeekSamp(p,b,ks);
	if (b) *b/=plr->fmt.nch;
}

CWavPlayer* CLoader::mono_create(CWavPlayer* plr)
{
	CMono* m=_new(CMono);
	if (m)
	{
		m->plr=plr;
		m->fmt=plr->fmt;
		m->fmt.nch=1;
		m->sb=plr->fmt.bps>>3;
		if (m->sb>7) return 0;
	}
	return m;
}

#define DITH_MAX 8

class CFloat: public CConverter
{
public:
	BYTE tbuf[TBUF_S];
	UINT sb,dst;//in bytes, not bits !!!!
	bool dither;
	UINT cur_ch;//for dithering
	double acc[DITH_MAX];//anyone uses more than 8 channels ?
	virtual UINT Read(void* buf,UINT buf_size,bool *ks);
	virtual void SeekSamp(UINT p,UINT* b,bool* ks);
};

void CFloat::SeekSamp(UINT p,UINT* b,bool* ks)
{
	plr->SeekSamp(p,b,ks);
	if (b) *b=MulDiv(*b,dst,sb);
}

static const float mul_tab[3]={(float)0x8000,(float)0x800000,(float)0x80000000};

static float convert_float16(WORD foo)
{
	union
	{
		float f;
		DWORD dw;
	} u;
	u.dw=0;
	if (foo&0x8000) u.dw|=0x80000000;//sign
	UINT bias,frac;//5:10
	bias=(foo>>10)&0x1F;
	frac=foo&0x3FF;
	bias+=(127-31);
	
	u.dw|=bias<<23;
	u.dw|=frac<<23-10;
	u.dw|=frac<<23-20;

	return u.f;
}

UINT CFloat::Read(void* buf,UINT buf_size,bool *ks)
{
	UINT nsam=buf_size/dst;
	BYTE* out=(BYTE*)buf;
	while(nsam)
	{
		int rd=nsam*sb;
		if (rd>TBUF_S) rd=TBUF_S;
		rd=plr->Read(tbuf,rd,ks);
		if (rd<=0) break;
		rd/=sb;
		nsam-=rd;
		double mul= (double)mul_tab[dst-1];
		double max=mul-1;
		double min=-mul;

		BYTE* src=tbuf;
		do {
			double val;
			if (sb==2) val=convert_float16(*(WORD*)src);
			else if (sb==4) val=*(float*)src;
			else if (sb==8) val=*(double*)src;
			
			val*=mul;
			if (val>max) val=max;
			else if (val<min) val=min;
			int v=(int)val;
			if (dither)
			{
				acc[cur_ch]+=val-(double)v;
				if (acc[cur_ch]>0.7) {val++;acc[cur_ch]-=1;}
				else if (acc[cur_ch]<-0.7) {val--;acc[cur_ch]+=1;}
				cur_ch++;
				cur_ch%=fmt.nch;
			}
			memcpy(out,&v,dst);
/*			switch(dst)
			{
			case 2:
				*(short*)out=v;
				break;
			case 4:
				*(long*)out=v;
				break;
			default:
				
			}*/

			src+=sb;
			out+=dst;
			rd--;
		} while(rd);
	}
	return out-(BYTE*)buf;
}

extern bool cfg_dither;

CWavPlayer* CLoader::float_create(CWavPlayer* p)
{
	UINT dst=(cfg_fdest+2)<<3;//16,24,32
	if ((dst!=16 && dst!=32 && dst!=24) || (p->fmt.bps!=32 && p->fmt.bps!=64 && p->fmt.bps!=16)) return 0;
	CFloat* c=_new(CFloat);
	if (c)
	{
		if (cfg_dither && c->fmt.nch<=DITH_MAX)
		{
			c->dither=1;			
		}
		c->plr=p;
		c->fmt=p->fmt;
		c->fmt.bps=dst;
		c->sb=p->fmt.bps>>3;
		c->dst=dst>>3;
	}
	return c;
}


class CReader : public WReader
{
	virtual int Open(char *url, bool *killswitch) {return 0;};
};


class CMemReader : public CReader
{
public:
	BYTE* mem;
	UINT sz;
	UINT pos;
	virtual int Read(char *buffer, UINT length, bool *killswitch) {if (length+pos>sz) length=sz-pos;memcpy(buffer,mem+pos,length);pos+=length;return length;}
	virtual int GetLength(void) {return sz;} 
	virtual int CanSeek(void) {return 1;};
	virtual  int Seek(UINT position, bool *killswitch) {pos=position;return 0;};
	virtual void Release() {if (mem) free(mem);delete this;}
};

WReader* hread_create(void* mem,UINT sz)
{
	CMemReader* h=_new(CMemReader);
	if (!h) return 0;
	h->mem=(BYTE*)mem;
	h->sz=sz;
	return h;
}
/*
CWavPlayer* mem_create(void* block,UINT size,FORMATSPEC *fmt,DWORD flags)
{
	WReader* h=hread_create(block,size);
	if (!h) return 0;
	CWavPlayer* p=raw_create(h,0,size,fmt,flags);
	if (!p) delete h;
	return p;

}
*/

class CShellReader : public CReader
{
public:
	WReader* r;
	virtual void Release() {if (r) r->Release();delete this;}	

};

class CPartializer : public CShellReader
{
public:
	UINT pos,start,size;
	int Read(char *buffer, UINT length, bool *ks)
	{
		if (*ks) return 0;
		if (pos+length>size) length=size-pos;
		if (length)
		{
			length=r->Read(buffer,length,ks);
			if (length>0) pos+=length;
		}
		return length;
	}
	int GetLength(void) {return size;} 
	int CanSeek(void) {return r->CanSeek();};
	int Seek(UINT position, bool *ks)
	{
		pos=position;
		return r->Seek(start+pos,ks);
	};
};

WReader* CLoader::partialize(UINT start,UINT size)
{
	if (*ks) return 0;
	UINT r_size=r->GetLength();
	if (start+size>r_size) return 0;
//	if (start==0 && size==r_size) return r;
	if (r->Seek(start,ks)) return 0;
	if (*ks) return 0;
	CPartializer* p=_new(CPartializer);
	if (!p) return 0;
	p->r=r;
	p->pos=0;
	p->start=start;
	p->size=size;
	return r=p;	
}

#define SBUF 0x800

class CSeekabilizer : public CShellReader
{
public:
	UINT pos,r_pos,buf_ofs;
	BYTE sbuf[SBUF];
	int GetLength(void) {return r->GetLength();} 
	int CanSeek(void) {return 0;};

	int Read(char *buffer, UINT length, bool *ks)
	{
		UINT pos_s=pos;
#define wr (pos-pos_s)
		if (pos<buf_ofs) return 0;
		if (pos>r_pos)
		{//seek forward
			BYTE foo[0x400];
			UINT d=pos-r_pos;
			pos=r_pos;
			while(d)
			{
				UINT rd=0x400;
				if (rd>d) rd=d;
				int rt=Read((char*)foo,rd,ks);
				if (rt<=0 || *ks) return 0;
				d-=rt;
			}
		}
		if (pos<r_pos)
		{
			UINT d=r_pos-pos;
			if (d>length) d=length;
			memcpy(buffer,sbuf+pos-buf_ofs,d);
			buffer+=d;
			pos+=d;
			if (wr==length) goto q;
		}
		//pos==r_pos
		if (length-wr>SBUF)
		{
			UINT d=length-wr-SBUF;
			UINT d1=r->Read(buffer,d,ks);
			if (*ks || (signed)d1<=0) goto q;
			r_pos+=d1;
			pos+=d1;
			buffer+=d1;
			if (d1!=d) goto q;
			d1=r->Read(buffer,SBUF,ks);
			if (*ks || d1<=0) goto q;
			memcpy(sbuf,buffer,d1);
			buf_ofs=r_pos;
			r_pos+=d1;
			pos+=d1;
		}
		else
		{
			UINT d=length-wr;
			UINT buf_used=r_pos-buf_ofs;
			UINT buf_free=SBUF-buf_used;
			if (buf_free<d)
			{//make free space in buffer
				UINT m=d-buf_free;
				buf_used-=m;
				memmove(sbuf,sbuf+m,buf_used);
				buf_ofs+=m;
			}
			int d1=r->Read(buffer,d,ks);
			if (*ks || d1<=0) goto q;
			memcpy(sbuf+buf_used,buffer,d1);
			r_pos+=d1;
			pos+=d1;
		}

q:
		return wr;
#undef wr
	}

	int Seek(UINT position, bool *ks)
	{
		pos=position;
		return 0;
	};
};

WReader* seekabilize(WReader* r)	//HACK - enable seeking on a reader that doesn't support it
{
	if (r->CanSeek()) return r;
	CSeekabilizer* s=_new(CSeekabilizer);
	if (s)
	{
		s->r=r;
	}
	return s;
}

int CLoader::cache()
{
	UINT sz=r->GetLength();
	if (sz==-1) {return 0;}
	void * b=malloc(sz);
	if (!b) {return 0;}
	if ((unsigned)r->Read((char*)b,sz,ks)!=sz || *ks) {free(b);return 0;}
	WReader * nr=hread_create(b,sz);
	if (nr) {r->Release();r=nr;return 1;}
	else return 0;
}

int CLoader::make_sure_we_can_seek()	//used by streamed vocs
{
	if (!r->CanSeek()) return cache();
	else return 1;
}

CChunkList* clist_create()
{
	CChunkList* l=_new(CChunkList);
	if (!l) return 0;
	l->data=(CHUNK*)malloc((l->data_size=0x20)*sizeof(CHUNK));
	if (!l->data) {delete l;return 0;}
	l->data_ptr=0;
	l->loop_ptr=cfg_exploop ? 0 : -1;
	return l;
}

void CChunkList::add(UINT start,UINT length)
{
	if (!length) return;
	if (data_ptr==data_size)
	{
		void* nd=realloc(data,(data_size<<=1)*sizeof(CHUNK));
		if (!nd) {data_size>>=1;return;}
		data=(CHUNK*)nd;
	}
	data[data_ptr].start=start;
	data[data_ptr].length=length;
	data[data_ptr++].filler_s=0;
}

void CChunkList::add_filler(UINT length,UINT f,UINT s)
{
	if (!length || !cfg_vocsil) return;
	if (data_ptr==data_size)
	{
		void* nd=realloc(data,data_size<<=1);
		if (!nd) {data_size>>=1;return;}
		data=(CHUNK*)nd;
	}
	data[data_ptr].filler=f;
	data[data_ptr].filler_s=s;
	data[data_ptr++].length=length;
}


void CChunkList::rel()
{
	if (data) free(data);
	delete this;
}


class CChunkalizer: public CShellReader
{
public:
	CChunkList* l;
	UINT size,cur_chunk,chunk_pos,global_pos;

	void Release() {if (l) l->rel();if (r) r->Release();delete this;}

	int Read(char *buffer, UINT length, bool *ks)
	{
		UINT wr=0;
		while(wr<length && !*ks)
		{
			if (cur_chunk>=l->data_ptr) break;
			UINT d=length-wr;
			UINT maxr=l->data[cur_chunk].length-chunk_pos;
			if (d>maxr) d=maxr;
			
			if (*ks) break;

			if (d)
			{
				if (l->data[cur_chunk].filler_s)
				{
					union
					{
						BYTE b[sizeof(int)];
						UINT i;
					} u;
					u.i=l->data[cur_chunk].filler;
					UINT n,t=l->data[cur_chunk].filler_s;
					char* out=buffer+wr;
					for(n=0;n<d;n++)
					{
						*(out++)=u.b[n%d];
					}
				}
				else d=r->Read(buffer+wr,d,ks);
				if (d<=0) break;
				wr+=d;
				chunk_pos+=d;
				if (*ks) break;
			}

			if (chunk_pos>=l->data[cur_chunk].length)
			{
				chunk_pos-=l->data[cur_chunk].length;
				cur_chunk++;
				if (cur_chunk>=l->data_ptr) break;
				if (!l->data[cur_chunk].filler_s)
				{
					if (r->Seek(l->data[cur_chunk].start,ks)) break;
				}
			}
		}
		global_pos+=wr;
		return wr;
	}

	int GetLength(void) {return size;}

	int CanSeek(void) {return r->CanSeek();};

	int Seek(UINT position, bool *ks)
	{
		if (position>size) return 1;
		if (position<global_pos)
		{
			global_pos=cur_chunk=chunk_pos=0;
		}		
		position-=global_pos;
		global_pos+=position;
		while(position>l->data[cur_chunk].length-chunk_pos)
		{
			position-=l->data[cur_chunk].length-chunk_pos;
			chunk_pos=0;
			cur_chunk++;
		}
		if (position)
		{
			chunk_pos=position;
			position=0;
		}

		if (l->data[cur_chunk].filler_s) return 0;
		else return r->Seek(l->data[cur_chunk].start+chunk_pos,ks);
	};
};


int CLoader::do_chunks(CChunkList* l)
{
	if (!l->data_ptr) {l->rel();return 0;}
	CChunkalizer * c=_new(CChunkalizer);
	if (!c) {l->rel();return 0;}
	c->l=l;
	c->r=r;
	r=c;
	UINT n;
	c->size=0;
	for(n=0;n<l->data_ptr;n++)
	{
		c->size+=l->data[n].length;
	}
	if (!l->data[0].filler_s)
	{
		if (c->r->Seek(l->data[0].start,ks)) return 0;
	}
	return 1;
}

void CChunkList::start_loop(UINT times)
{
	if (loop_ptr==-1) return;
	loop_ptr++;
	if (loop_ptr>=LOOP_MAX) return;
	loop_starts[loop_ptr-1]=data_ptr;
	loop_counts[loop_ptr-1]=times;
}

void CChunkList::finish_loop(UINT times)
{
	if (!loop_ptr || loop_ptr==-1) return;
	loop_ptr--;
	if (loop_ptr>=LOOP_MAX) return;
	if (loop_starts[loop_ptr]==data_ptr) return;
	UINT end=data_ptr,start=loop_starts[loop_ptr];
	if (!times)
		if (!(times=loop_counts[loop_ptr])) return;	
	while(--times)
	{
		UINT n;
		for(n=start;n<end;n++)
		{
			if (data[n].filler_s) add_filler(data[n].length,data[n].filler,data[n].filler_s);
			else add(data[n].start,data[n].length);
		}
	}	
}

int CWavPlayer::processData(MediaInfo *,ChunkList* cl,char * tempbuf,UINT tempsize,bool * ks)
{
	int eof=0;
	UINT len=Read(tempbuf,tempsize,ks);
	if(len!=tempsize) eof=1;

	if (len>0)
	{
		ChunkInfosI *ci=new ChunkInfosI();
		ci->addInfo("srate",fmt.sr);
		ci->addInfo("bps",fmt.bps);
		ci->addInfo("nch",fmt.nch);
		cl->setChunk("PCM",tempbuf,len,ci);
		if (fmt.chan) cl->setChunk("SPEAKER_SETUP",&fmt.chan,4,0);

	}

	return eof ? 0 : 1;
}



void * z_malloc(UINT x)
{
	void *ret=malloc(x);
	if (ret) memset(ret,0,x);
	return ret;
}
void * newhack(void * o,UINT s)
{
	if (o) memset((int*)o+1,0,s-sizeof(int));
	return o;
}

char ERROR_FILE_DAMAGED[]="File is damaged.";