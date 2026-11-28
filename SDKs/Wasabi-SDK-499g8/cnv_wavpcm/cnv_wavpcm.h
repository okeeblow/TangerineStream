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
#ifndef _CNV_WAVE_H
#define _CNV_WAVE_H

#include <studio/wac.h>
#include <studio/services/svc_mediaconverter.h>
#include <bfc/attribs/attrint.h>
#include <bfc/attribs/attrbool.h>

DWORD _inline rev32(DWORD X) {return ((((DWORD)(X)&0xFF)<<24)|(((DWORD)(X)&0xFF00)<<8)|(((DWORD)(X)&0xFF0000)>>8)|(((DWORD)(X)&0xFF000000)>>24));}
WORD _inline rev16(WORD X) {return (X>>8)|(X<<8);}


class StringPrintChan : public String
{
public:
	StringPrintChan(int x) : String(x==1 ? "mono" : x==2 ? "stereo" : StringPrintf("%ich",x)) {}
};

class StringPrintFreq : public String
{
public:
	StringPrintFreq(int x) 
		: String(
		x<1000 ? StringPrintf("%iHz",x) : 
	x%1000==0 ? StringPrintf("%ikHz",x/1000) : 
	StringPrintf("%i.%ikHz",(x+50)/1000,((x+50)/100)%10)
		) {}
};

class StringPrintInfo : public StringPrintf
{
public:
	StringPrintInfo(int bytes_per_sec,int channels,int srate)
		: StringPrintf("%ukbps %s %s",(bytes_per_sec*8+500)/1000,StringPrintChan(channels),StringPrintFreq(srate))
	{}
};


class converter_base : public svc_mediaConverterI
{
protected:
	int need_seek;
	converter_base() {need_seek=-1;}
	virtual int corecb_onSeeked(int newpos)
	{
		need_seek=newpos;
		return 1;
	}

	virtual const char *getConverterTo() { return "PCM"; }
};

void fix_byte_order(void * data,int size,int bps);
void fix_8bit_sign(void * data,int size);

class ima_adpcm_decoder
{
private:
	int sample,index;
public:
	ima_adpcm_decoder() {sample=index=0;}
	void reset(int s,int i) {sample=s;index=i;}
	int decode(int data);
};



/*

leet trick.
to avoid having to manually keep central registry of all our services in cnv_wavpcm.cpp to register them all
we use a class that keeps track of all its instances
then just converter_make_service() on whatever converter class we want to add

NOTE
this is supposed to work with static instances only
dont create service_factory_base stuff dynamically

*/

class service_factory_base
{
public:
	virtual waServiceFactoryI * getPtr()=0;
	service_factory_base * next;
	static service_factory_base * list;
	service_factory_base() {next=list;list=this;}
};

template<class foo,class bar>
class service_factory : public service_factory_base
{
private:
	waServiceFactoryT<foo,bar> m_factory;
	virtual waServiceFactoryI * getPtr() {return &m_factory;}
};

template<class foo>
class service_factory_converter : public service_factory<svc_mediaConverter,foo> {};

#define converter_make_service(foo) static service_factory_converter<foo> g_svc_##foo##__FILE__##__LINE__;

#endif
