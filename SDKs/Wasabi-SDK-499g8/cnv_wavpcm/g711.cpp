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

#ifdef EXTRA_DECODERS

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

int
alaw2linear(
	unsigned char	a_val)
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

#define	BIAS		(0x84)		/* Bias for linear code. */

int
ulaw2linear(
	unsigned char	u_val)
{
	int		t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

typedef int (*cvt_func)(BYTE);

#define TBUF 0x400

class CG711Player : public CWavPlayerRd
{
public:
	cvt_func func;
	bool inv;
	BYTE tbuf[0x400];
	UINT Read(void* buf,UINT buf_size,bool* ks)
	{
		short* out=(short*)buf;
		UINT rd=0;
		while(rd<buf_size && !*ks)
		{
			UINT d=buf_size>>1;
			if (d>TBUF) d=TBUF;
			if (d) d=r->Read((char*)tbuf,d,ks);
			if (!d || (d&0x80000000) || *ks) break;
			UINT n;
			for(n=0;n<d;n++)
			{
				BYTE b=tbuf[n];
				if (inv) b=inv_bits(b);
				*(out++)=func(b);
			}
			rd+=d<<1;
		};
		return rd;
	}
	UINT GetLengthSamp() {return r->GetLength()/fmt.nch;}
	void SeekSamp(UINT p,UINT* b,bool* ks)
	{
		p*=fmt.nch;
		if (b) *b=p<<1;
		r->Seek(p,ks);
	}
	void GetInfo(WAVEINFO* wi)
	{
		FORMATSPEC f;
		f.bps=8;
		f.nch=fmt.nch;
		f.sr=fmt.sr;
		char* tag;
		if (func==alaw2linear) tag="A-law";
		else tag="mu-law";
		do_info(wi,&f,tag,GetLengthMS());
	}
};


static CWavPlayer* g711_create(WReader* r,UINT nch,UINT sr,cvt_func cfunc,bool inv)
{
	CG711Player* p=_new(CG711Player);
	if (p)
	{
		p->inv=inv;
		p->r=r;
		p->fmt.nch=nch;
		p->fmt.sr=sr;
		p->fmt.bps=16;
		p->func=cfunc;
	}
	return p;
}

CWavPlayer* CLoader::mulaw_create(UINT nch,UINT sr,bool inv)
{
	return g711_create(r,nch,sr,ulaw2linear,inv);
}

CWavPlayer* CLoader::alaw_create(UINT nch,UINT sr,bool inv)
{
	return g711_create(r,nch,sr,alaw2linear,inv);
}
#endif