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
#ifndef _CNV_DS2_H
#define _CNV_DS2_H

#include <studio/wac.h>
#include <bfc/attribs/cfgitemi.h>
#include <bfc/attribs/attribs.h>

#include <studio/services/svc_mediaconverter.h>
#include <studio/services/servicei.h>
#include <core/corecb.h>

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include "ds2.h"

#define WACNAME WACcnv_ds2

class WACNAME : public WAComponentClient {
public:
  WACNAME();
  virtual ~WACNAME();

  virtual const char *getName() { return "DirectSound Output"; };
  virtual GUID getGUID();

  virtual void onCreate();
  virtual void onDestroy();
  
  virtual int getDisplayComponent() { return FALSE; };

  virtual CfgItem *getCfgInterface(int n) { return this; }
};


class cnvDS2 : public svc_mediaConverterI {
public:
  cnvDS2();
  virtual ~cnvDS2();

  // service
  static const char *getServiceName() { return "DirectSound Output"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(chunktype && !STRICMP(chunktype,"pcm")) return 1;
    return 0;
  }
  virtual const char *getConverterTo() { return "OUTPUT:DirectSound"; }

  virtual int getInfos(MediaInfo *infos);

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void);

  virtual int isSelectableOutput(void) { return 1; }

  // callbacks
  virtual int corecb_onSeeked(int newpos);

	virtual int sortPlacement(const char *oc);

	virtual int corecb_onVolumeChange(int v);
	virtual int corecb_onPanChange(int v);
	virtual int corecb_onAbortCurrentSong();
	virtual int corecb_onPaused();
	virtual int corecb_onUnpaused();
	virtual int corecb_onEndOfDecode();
private:
	DS2 * m_ds2;
	UINT sr,nch,bps,chan;
	bool ds2_paused;
	UINT fadenow;
	bool use_vol;
	bool use_pitch;
	double pitch_set;
};


#endif
