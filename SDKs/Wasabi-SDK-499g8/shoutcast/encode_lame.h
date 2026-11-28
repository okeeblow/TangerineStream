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
#ifndef _ENCODE_LAME_H
#define _ENCODE_LAME_H

#include "../studio/services/svc_mediaconverter.h"
#include "../studio/services/servicei.h"
#include "../studio/corecb.h"
#include "../studio/wac.h"
#include "../attribs/cfgitemi.h"
#include "../attribs/attrint.h"

#include "shoutcastwnd.h"
#include "encoder.h"
#include "server.h"

#include "blademp3encdll.h"

class ContNotify : public ContWnd {
  virtual int childNotify(RootWnd *child, int msg, int param1=0, int param2=0) {
    return getParent()->childNotify(child,msg,param1,param2);
  }
};

class EncoderLame : public Encoder {
public:
  EncoderLame();
  virtual ~EncoderLame();

  // service
  static const char *getServiceName() { return "LAME PCM to MP3 encoder"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(chunktype && !STRICMP(chunktype,"PCM")) return 1;
    return 0;
  }
  virtual const char *getConverterTo() { return "MP3"; }

  virtual int getInfos(MediaInfo *infos) {
    infos->setDataInt("bitrate",vbr?vbrbr:bitrate);
    return 0; 
  }

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void) { return 0; }

  void initConfig(ContWnd *configwnd, int x, int y);
  void updateConfig();
  void getConfig();
  void loadConfig(const char *pre);
  void saveConfig(const char *pre);
  void removeConfig();
private:
  HBE_STREAM hbs;
  HINSTANCE lamedll;
  BEINITSTREAM beInitStream;
  BECLOSESTREAM beCloseStream;
  BEENCODECHUNK beEncodeChunk;
  DWORD lame_bs,lame_ss;

  int quality,bitrate,mode;
  BOOL vbr;
  int vbrbr,vbrquality;
  int encodenecessary;
  int samplerate;

  int reinit;

  MemBlock<char> buf;
//  MemBlock<char> retmp3buf;
  Buffer retmp3buf;

  Buffer encodebuf;

  // config gui stuff
  ContNotify cont;
  ContWnd *contparent;
  TextBar qualitytext,brtext,modetext,vbrtext,vbrbrtext,vbrqualtext,entext,sratetext;
  DropList qualitydrop,brdrop,modedrop,vbrbrdrop,vbrqualdrop,sratedrop;
  CheckWnd vbrcheck,encheck;
};

#endif