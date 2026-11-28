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

void CAdpcmPlayer::GetInfoInternal(WAVEINFO* wi,char* tag)
{
	FORMATSPEC f;
	f.bps=bits;
	f.sr=fmt.sr;
	f.nch=fmt.nch;
	do_info(wi,&f,tag,GetLengthMS());
}


UINT CAdpcmPlayer::read_buf(void* b,UINT s,bool* ks)
{
	if (eof) return 0;
	BYTE* out=(BYTE*)b;
	UINT n;
	bufpos=(bufpos+7)&~7;
	for(n=0;n<s;n++)
	{
		out[n]=read_byte(ks);
		if (eof || *ks) break;
	}
	return n;

}

void CAdpcmPlayer::checkbuf(bool* ks)
{
	if (eof) return;
	if (!data || bufpos==data)
	{
		data=r->Read((char*)buf,ADPCM_BUF,ks);
		if (data==-1 || data==0) {eof=1;return;}
		data<<=3;
		bufpos=0;
	}
}

UINT CAdpcmPlayer::GetLengthSamp()
{
	return MulDiv(r->GetLength(),8,bits*fmt.nch);
}

UINT CAdpcmPlayer::read_bits(UINT bits,bool* ks)
{
	UINT ret=0;
	UINT p=0;
	while(p<bits)
	{
		checkbuf(ks);
		if (*ks || eof) return 0;
		if ( buf[bufpos>>3] & ( 1<<(bufpos&7) )) ret|=1<<p;
		p++;
		bufpos++;
	}
	return ret;
}

void CAdpcmPlayer::flush(UINT p,bool* ks)
{
	r->Seek(p,ks);
	eof=0;
	data=bufpos=0;
}