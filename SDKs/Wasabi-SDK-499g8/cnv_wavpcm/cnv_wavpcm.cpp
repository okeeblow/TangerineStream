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

#define WACNAME WACcnv_wave

// {98C54B21-75E3-411c-BD33-2F1307D3E141}
static const GUID guid = 
{ 0x98c54b21, 0x75e3, 0x411c, { 0xbd, 0x33, 0x2f, 0x13, 0x7, 0xd3, 0xe1, 0x41 } };

service_factory_base * service_factory_base::list=0;

class WACNAME : public WAComponentClient{
public:
	WACNAME()
	{
#ifdef FORTIFY
		FortifySetName("cnv_wavpcm.wac");
		FortifyEnterScope();
#endif
	}
	virtual ~WACNAME()
	{
#ifdef FORTIFY
		FortifyLeaveScope();
#endif
	}

	virtual const char *getName() {return "Waveform Decoder";}

	virtual GUID getGUID() {return guid;}
	virtual void onCreate()
	{
		service_factory_base * ptr = service_factory_base::list;
		while(ptr)
		{
			api->service_register(ptr->getPtr());
			ptr=ptr->next;
		}

		api->core_registerExtension("*.wav","WAV Files","Audio");
		api->core_registerExtension("*.aiff;*.aif","AIFF Files","Audio");
		api->core_registerExtension("*.au;*.snd","AU/SND Files","Audio");
		api->core_registerExtension("*.voc","VOC Files","Audio");
//		api->core_registerExtension("*.iff","IFF Files","Audio");
//		api->core_registerExtension("*.svx","8SVX Files","Audio");
//		api->core_registerExtension("*.aud","AUD Files","Audio");
	}

	virtual void onDestroy()
	{
		service_factory_base * ptr = service_factory_base::list;
		while(ptr)
		{
			api->service_deregister(ptr->getPtr());
			ptr=ptr->next;
		}
	}

	virtual int getDisplayComponent() { return FALSE; };

	virtual CfgItem *getCfgInterface(int n) { return this; }
};

static WACNAME wac;
WAComponentClient *the = &wac;