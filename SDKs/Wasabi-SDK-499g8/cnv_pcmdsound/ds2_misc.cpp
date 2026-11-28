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
#include "ds2.h"

void DS2::SoundBlockList::AddBlock(void * data,UINT size)
{
	SoundBlock * b = remove(b_free);
	if (!b) b=new SoundBlock;

	b->SetData(data,size);

	mount2(b_first,b,b_last);

	data_buffered+=size;
#ifdef USE_LOG
	char boo[256];
	wsprintf(boo,"AddBlock: %u, +%u",data_buffered,size);
	log_write(boo);
#endif
}

UINT DS2::SoundBlockList::DumpBlocks(void * out1,UINT size1)
{
	char * p1=(char*)out1;
	UINT rv=0;
	while(b_last && size1)
	{
		if (size1)
		{
			UINT d=b_last->Dump(p1,size1);
			p1+=d;
			size1-=d;
			rv+=d;
		}
		if (b_last->GetDataSize()==0)
		{
			mount(b_free,removelast(b_first,b_last));
		}
	}
	if (size1) memset(p1,silence_filler,size1);

	data_buffered-=rv;
	return rv;
}

void DS2::SoundBlockList::Reset()
{
	if (b_first)
	{
		connect(b_last,b_free);
		b_free=b_first;
	}

	b_first=0;
	b_last=0;
	data_buffered=0;
}

DS2::SoundBlockList::~SoundBlockList()
{
	SoundBlock * p;
	while(b_first)
	{
		p=b_first;
		b_first=b_first->next;
		delete p;
	}
	while(b_free)
	{
		p=b_free;
		b_free=b_free->next;
		delete p;
	}
}

void DS2config::SetErrorCodeMsg(const char * msg,DWORD code)
{
	if (code)
	{
		char boo[512];
		wsprintf(boo,"%s\nError code: %08X",msg,code);
		SetError(boo);
	}
	else SetError(msg);
}


UINT DS2::bytes2ms(UINT bytes) {return MulDiv(bytes,1000,fmt_mul);}
UINT DS2::ms2bytes(UINT ms) {return MulDiv(ms,fmt_mul,1000);}
