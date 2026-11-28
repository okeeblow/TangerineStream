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
#include "encode_ogg.h"

EncoderOgg::EncoderOgg() {
  vbr=false;
  vbrquality=0.5;
//  bitrate=160;
  bitrate=24;
  minbitrate=1;
//  maxbitrate=320;
  maxbitrate=32;
  
  reinit=1;

  vorbisdll=LoadLibrary("wacs/libvorbis.dll");
  if(vorbisdll) {
    vorbisInfoInit=(VINFOINIT)GetProcAddress(vorbisdll,"vorbis_info_init");
    vorbisEncodeInit=(VENCODEINIT)GetProcAddress(vorbisdll,"vorbis_encode_init");
    vorbisEncodeInitVbr=(VENCODEINITVBR)GetProcAddress(vorbisdll,"vorbis_encode_init_vbr");
    vorbisAnalysisInit=(VANALYSISINIT)GetProcAddress(vorbisdll,"vorbis_analysis_init");
    vorbisAnalysisHeaderOut=(VANALYSISHEADEROUT)GetProcAddress(vorbisdll,"vorbis_analysis_headerout");
    vorbisAnalysisBuffer=(VANALYSISBUFFER)GetProcAddress(vorbisdll,"vorbis_analysis_buffer");
    vorbisAnalysisWrote=(VANALYSISWROTE)GetProcAddress(vorbisdll,"vorbis_analysis_wrote");
    vorbisAnalysis=(VANALYSIS)GetProcAddress(vorbisdll,"vorbis_analysis");
    vorbisAnalysisBlockOut=(VANALYSISBLOCKOUT)GetProcAddress(vorbisdll,"vorbis_analysis_blockout");
    vorbisBlockInit=(VBLOCKINIT)GetProcAddress(vorbisdll,"vorbis_block_init");
    vorbisCommentInit=(VCOMMENTINIT)GetProcAddress(vorbisdll,"vorbis_comment_init");
    vorbisBitrateAddBlock=(VBITRATEADDBLOCK)GetProcAddress(vorbisdll,"vorbis_bitrate_addblock");
    vorbisBitrateFlushPacket=(VBITRATEFLUSHPACKET)GetProcAddress(vorbisdll,"vorbis_bitrate_flushpacket");
    oggStreamInit=(OSTREAMINIT)GetProcAddress(vorbisdll,"ogg_stream_init");
    oggStreamPacketIn=(OSTREAMPACKETIN)GetProcAddress(vorbisdll,"ogg_stream_packetin");
    oggStreamFlush=(OSTREAMFLUSH)GetProcAddress(vorbisdll,"ogg_stream_flush");
    oggStreamPageOut=(OSTREAMPAGEOUT)GetProcAddress(vorbisdll,"ogg_stream_pageout");
  }
}

EncoderOgg::~EncoderOgg() {
  if(vorbisdll) FreeLibrary(vorbisdll);
}

int EncoderOgg::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  if(!vorbisdll) return 1;

  if(reinit) {
    outbuf.reset();
    
    vorbisInfoInit(&vi);
    if(!vbr) vorbisEncodeInit(&vi,2,44100,maxbitrate*1024,bitrate*1024,minbitrate*1024);
    else vorbisEncodeInitVbr(&vi,2,44100,vbrquality);

    vorbisCommentInit(&vc);
//    vorbis_comment_add_tag(&vc,"ENCODER","encoder_example.c");

    /* set up the analysis state and auxiliary encoding storage */
    vorbisAnalysisInit(&vd,&vi);
    vorbisBlockInit(&vd,&vb);
    /* set up our packet->stream encoder */
    /* pick a random serial number; that way we can more likely build
     chained streams just by concatenation */
    srand(time(NULL));
    oggStreamInit(&os,rand());
    
    {
      ogg_packet header;
      ogg_packet header_comm;
      ogg_packet header_code;

      vorbisAnalysisHeaderOut(&vd,&vc,&header,&header_comm,&header_code);
      oggStreamPacketIn(&os,&header); /* automatically placed in its own page */
      oggStreamPacketIn(&os,&header_comm);
      oggStreamPacketIn(&os,&header_code);

	  /* We don't have to write out here, but doing so makes streaming 
	  * much easier, so we do, flushing ALL pages. This ensures the actual
	  * audio data will start on a new page
	  */
  	  while(1){
  	  	int result=oggStreamFlush(&os,&og);
		    if(result==0)break;
        outbuf.add(og.header,og.header_len);
        outbuf.add(og.body,og.body_len);
	    }
    }
    reinit=0;
  }

  Chunk *c=chunk_list->getChunk("PCM");
  if(!c) return 1;

  int nch=c->getInfo("nch");
  int srate=c->getInfo("srate");
  int bps=c->getInfo("bps");
  char *pcmadr=(char *)c->getData();
  int bytes=c->getSize();
  if(!bytes) return 1;

  /* expose the buffer to submit data */
  float **buffer=vorbisAnalysisBuffer(&vd,bytes/4);
      
  /* uninterleave samples */
  int i;
  for(i=0;i<bytes/4;i++){
  	buffer[0][i]=((pcmadr[i*4+1]<<8) | (0x00ff&(int)pcmadr[i*4]))/32768.f;
	  buffer[1][i]=((pcmadr[i*4+3]<<8) | (0x00ff&(int)pcmadr[i*4+2]))/32768.f;
  }
    
  /* tell the library how much we actually submitted */
  vorbisAnalysisWrote(&vd,i);

  /* vorbis does some data preanalysis, then divvies up blocks for
       more involved (potentially parallel) processing.  Get a single
       block for encoding now */
  while(vorbisAnalysisBlockOut(&vd,&vb)==1){

    /* analysis, assume we want to use bitrate management */
    vorbisAnalysis(&vb,NULL);
    vorbisBitrateAddBlock(&vb);

    while(vorbisBitrateFlushPacket(&vd,&op)){
	
    	/* weld the packet into the bitstream */
	    oggStreamPacketIn(&os,&op);
	
	    /* write out pages (if any) */
      int eos=0;
	    while(!eos){
	      int result=oggStreamPageOut(&os,&og);
	      if(result==0)break;
        outbuf.add(og.header,og.header_len);
        outbuf.add(og.body,og.body_len);
	  
	      /* this could be set above, but for illustrative purposes, I do
	        it here (to show that vorbis does know where the stream ends) */
	  
//	      if(ogg_page_eos(&og))eos=1;
	    }
    }
  }

  int os=outbuf.getSize();
  if(os) {
    outblock.setSize(os);
    outbuf.get(outblock.getMemory(),os);
    chunk_list->setChunk("OGG",outblock.getMemory(),os);
  }

  return 1;
}

void EncoderOgg::initConfig(ContWnd *configwnd, int x, int y) {
}

void EncoderOgg::removeConfig() {
}

void EncoderOgg::updateConfig() {
}

void EncoderOgg::getConfig() {
  reinit=1;
}

void EncoderOgg::loadConfig(const char *pre) {
}

void EncoderOgg::saveConfig(const char *pre) {
}
