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

#ifdef DS2_HAVE_DEVICES

static const GUID NULL_GUID;

BOOL WINAPI DsDevEnum::DSEnumCallback(LPGUID guid,LPCTSTR desc,LPCTSTR,DS_DEV *** var)
{
	DS_DEV * dev=new DS_DEV;
	dev->name=strdup(desc);
	dev->guid=guid ? *guid : NULL_GUID;

	**var = dev;
	*var=&dev->next;

	return 1;
}

DsDevEnum::DsDevEnum()
{
	DS2::InitDLL();
	ds_devs=0;
	DS_DEV ** dev_ptr=&ds_devs;

	if (DS2::pDirectSoundEnumerate) DS2::pDirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback,&dev_ptr);

	*dev_ptr=0;
	pDev=ds_devs;
}

DsDevEnum::~DsDevEnum()
{
	DS_DEV* dev=ds_devs;
	while(dev)
	{
		DS_DEV * next=dev->next;
		free(dev->name);
		delete dev;
		dev=next;
	}
//	ds_devs=0;
}



bool DsDevEnum::FindGuid(const GUID & g)
{
	pDev=ds_devs;
	while(pDev)
	{
		if (pDev->guid==g) return 1;
		pDev=pDev->next;
	}
	return 0;
}

bool DsDevEnum::FindDefault()
{
	return FindGuid(NULL_GUID);
}

bool DsDevEnum::FindName(const char * n)
{
	pDev=ds_devs;
	if (!n) return 1;
	while(pDev)
	{
		if (!stricmp(pDev->name,n)) return 1;
		pDev=pDev->next;
	}
	return 0;
}
/*
const GUID  DsDevEnum::GuidFromName(const char* name)
{
	return FindName(name) ? GetGuid() : NULL_GUID; 
}

const char * DsDevEnum::NameFromGuid(const GUID & g, char * def)
{
	return FindGuid(g) ? GetName() : def;
}
*/
const GUID DsDevEnum::GetGuid()
{
	return pDev ? pDev->guid : NULL_GUID;
}

static bool _getcaps(IDirectSound * pDS,LPDSCAPS pCaps,DWORD * speakercfg)
{
	bool rv=1;
	if (pCaps)
	{
		memset(pCaps,0,sizeof(*pCaps));
		pCaps->dwSize=sizeof(*pCaps);
		if (FAILED(pDS->GetCaps(pCaps))) rv=0;
	}
	if (speakercfg)
	{
		if (FAILED(pDS->GetSpeakerConfig(speakercfg))) rv=0;
	}
	return rv;
}

bool DsDevEnum::GetCapsFromGuid(const GUID *dev,LPDSCAPS pCaps,DWORD * speakercfg)
{
	bool rv=0;
	if (!dev) dev=&NULL_GUID;


	if (DS2::TryGetDevCaps(dev,pCaps,speakercfg)) rv=1;
	else
	{
		IDirectSound * l_pDS=0;
		if (SUCCEEDED(DS2::myDirectSoundCreate(dev,&l_pDS)) && l_pDS)
		{
			rv=_getcaps(l_pDS,pCaps,speakercfg);
			l_pDS->Release();
		}
	}
	return rv;
}

bool DS2::TryGetDevCaps(const GUID *g,LPDSCAPS pCaps,DWORD * speakercfg)
{
	bool rv=0;
	SYNC_IN();
	if (!g) g=&NULL_GUID;
	if (pDS && GetCurDev()==*g)
	{
		rv=_getcaps(pDS,pCaps,speakercfg);
	}
	SYNC_OUT();
	return rv;
}
#endif