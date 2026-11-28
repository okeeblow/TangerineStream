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
#ifndef _ENCODE_OGG_H
#define _ENCODE_OGG_H

#include "../studio/services/svc_mediaconverter.h"
#include "../studio/services/servicei.h"
#include "../studio/corecb.h"
#include "../studio/wac.h"
#include "../attribs/cfgitemi.h"
#include "../attribs/attrint.h"

#include "shoutcastwnd.h"
#include "encoder.h"
#include "server.h"

#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"

#include "encode_lame.h" // for ContNotify

typedef void	(*VINFOINIT)			(vorbis_info *);
typedef int (*VENCODEINIT)			(vorbis_info *,long,long,long,long,long);
typedef int (*VENCODEINITVBR)			(vorbis_info *,long,long,float);
typedef int	(*VANALYSISINIT)			(vorbis_dsp_state *,vorbis_info *);
typedef int	(*VBLOCKINIT)			(vorbis_dsp_state *,vorbis_block *);
typedef void	(*VCOMMENTINIT)			(vorbis_comment *v);
typedef int	(*VANALYSISHEADEROUT)			(vorbis_dsp_state *,vorbis_comment *,ogg_packet *,ogg_packet *,ogg_packet *);
typedef float **(*VANALYSISBUFFER)			(vorbis_dsp_state *,int);
typedef int (*VANALYSISWROTE)			(vorbis_dsp_state *,int);
typedef int (*VANALYSISBLOCKOUT)			(vorbis_dsp_state *,vorbis_block *);
typedef int (*VANALYSIS)			(vorbis_block *,ogg_packet *);
typedef int (*VBITRATEADDBLOCK) (vorbis_block *);
typedef int (*VBITRATEFLUSHPACKET) (vorbis_dsp_state *,ogg_packet *);

typedef int	(*OSTREAMINIT)			(ogg_stream_state*, int);
typedef int	(*OSTREAMPACKETIN)			(ogg_stream_state *, ogg_packet *);
typedef int	(*OSTREAMFLUSH)			(ogg_stream_state *, ogg_page *);
typedef int	(*OSTREAMPAGEOUT)			(ogg_stream_state *, ogg_page *);

class EncoderOgg : public Encoder {
public:
  EncoderOgg();
  virtual ~EncoderOgg();

  // service
  static const char *getServiceName() { return "Vorbis PCM to OGG encoder"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(chunktype && !STRICMP(chunktype,"PCM")) return 1;
    return 0;
  }
  virtual const char *getConverterTo() { return "OGG"; }

  virtual int getInfos(MediaInfo *infos) {
    infos->setDataInt("bitrate",bitrate);
    return 0; 
  }

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void) { return 0; }

  virtual const char *getContentType() { return "application/x-ogg"; }

  void initConfig(ContWnd *configwnd, int x, int y);
  void updateConfig();
  void getConfig();
  void loadConfig(const char *pre);
  void saveConfig(const char *pre);
  void removeConfig();
private:
  BOOL vbr;
  float vbrquality;
  int bitrate,minbitrate,maxbitrate;
  int useminbitrate,usemaxbitrate;

  int reinit;

  ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */

  vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
  vorbis_comment   vc; /* struct that stores all the user comments */

  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */



  HINSTANCE vorbisdll;
  VINFOINIT vorbisInfoInit;
  VENCODEINIT vorbisEncodeInit;
  VENCODEINITVBR vorbisEncodeInitVbr;
  VANALYSISINIT vorbisAnalysisInit;
  VANALYSISHEADEROUT vorbisAnalysisHeaderOut;
  VANALYSISBUFFER vorbisAnalysisBuffer;
  VANALYSISWROTE vorbisAnalysisWrote;
  VANALYSIS vorbisAnalysis;
  VANALYSISBLOCKOUT vorbisAnalysisBlockOut;
  VBLOCKINIT vorbisBlockInit;
  VCOMMENTINIT vorbisCommentInit;
  VBITRATEADDBLOCK vorbisBitrateAddBlock;
  VBITRATEFLUSHPACKET vorbisBitrateFlushPacket;
  OSTREAMINIT oggStreamInit;
  OSTREAMPACKETIN oggStreamPacketIn;
  OSTREAMFLUSH oggStreamFlush;
  OSTREAMPAGEOUT oggStreamPageOut;

  Buffer outbuf;
  MemBlock<char> outblock;
/*  // config gui stuff
  ContNotify cont;
  ContWnd *contparent;
  TextBar qualitytext,brtext,modetext,vbrtext,vbrbrtext,vbrqualtext,entext,sratetext;
  DropList qualitydrop,brdrop,modedrop,vbrbrdrop,vbrqualdrop,sratedrop;
  CheckWnd vbrcheck,encheck;*/
};

#endif