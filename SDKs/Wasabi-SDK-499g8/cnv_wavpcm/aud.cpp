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

//WS ADPCM stuff

#include "aud.h"


static char WSTable2bit[]={-2,-1,0,1};

static char WSTable4bit[]={-9, -8, -6, -5, -4, -3, -2, -1,  0,  1,  2,  3,  4,  5,  6,  8};

static void WS_decode(BYTE* src,BYTE* out,DWORD os,DWORD s)
{
	if (s==os)
	{
		memcpy(out,src,os);
		return;
	}
	UINT ip=0,op=0;
	BYTE code;
	int sample=0x80;
	while(op<os)
	{
		code=src[ip++];
		BYTE fmt=code>>6;
		BYTE cnt=(code&0x3F);
		switch(fmt)
		{
		case 0:
			cnt++;
			while(cnt)
			{
				code=src[ip++];
				UINT n;
				for(n=0;n<4;n++)
				{
					sample+=WSTable2bit[code&3];
					if (sample<0) sample=0;
					else if (sample>255) sample=255;
					out[op++]=(BYTE)sample;
					code>>=2;
				}
				cnt--;
			}
			break;
		case 1:
			cnt++;
			while(cnt)
			{
				code=src[ip++];
				UINT n;
				for(n=0;n<2;n++)
				{
					sample+=WSTable4bit[code&0xF];
					if (sample<0) sample=0;
					else if (sample>255) sample=255;
					out[op++]=(BYTE)sample;
					code>>=4;
				}
				cnt--;
			}
			break;
		case 2:
			if (cnt & 0x20)
			{
				sample+=((char)(cnt<<3))>>3;
				if (sample<0) sample=0;
				else if (sample>255) sample=255;
				out[op++]=sample;
			}
			else
			{
				cnt++;
				while(cnt)
				{
					out[op++]=src[ip++];
					cnt--;
				}
				sample=out[op-1];
			}
			break;
		case 3:
			cnt++;
			while(cnt)
			{
				out[op++]=(BYTE)sample;
				cnt--;
			}
			break;

		}										
	}
}

#define TBUF_SIZE 0x4000


CWavPlayer* CLoader::aud_create_ws()
{
	AUD_header1 hd;
	chunk_header chd;
	CWriteBuf* wb=wb_create();
	BYTE * tbuf=(BYTE*)malloc(TBUF_SIZE);
	UINT rd=0;
	if (!tbuf) goto fail;
	if (r->Read((char*)&hd,sizeof(hd),ks)!=sizeof(hd)) goto fail;
	if (!wb) goto fail;
	while(rd<hd.Size)
	{
		if (*ks) goto fail;
		if (r->Read((char*)&chd,sizeof(chd),ks)!=sizeof(chd)) goto fail;
		rd+=sizeof(chd);
		if (chd.ID!=0x0000DEAF || chd.Size+chd.OutSize>TBUF_SIZE) goto fail;
		if (r->Read((char*)tbuf,chd.Size,ks)!=chd.Size || *ks) goto fail;
		rd+=chd.Size;
		BYTE* out=tbuf+chd.Size;
		WS_decode(tbuf,out,chd.OutSize,chd.Size);
		wb->write(out,chd.OutSize);
	}
	free(tbuf);
	tbuf=0;
	{
		UINT sz;
		void* ptr=wb->finish(&sz);
		wb=0;
		if (!ptr) goto fail;
		r->Release();
		r=hread_create(ptr,sz);
		if (!r) {free(ptr);goto fail;}
		FORMATSPEC fmt;
		fmt.sr=22050;
		fmt.nch=1;
		fmt.bps=8;
		return raw_create(&fmt,0);
	}
	
fail:
	if (wb) wb->freebuf();
	if (tbuf) free(tbuf);
	return 0;
}