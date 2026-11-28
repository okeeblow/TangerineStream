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
#include "encode_lame.h"

EncoderLame::EncoderLame() {
  hbs=NULL;
  quality=LQP_NORMAL_QUALITY;
  mode=BE_MP3_MODE_STEREO;
  //mode=BE_MP3_MODE_JSTEREO;
  //mode=BE_MP3_MODE_MONO;
  bitrate=128;
  vbr=false;
  vbrbr=320;
  vbrquality=5;
  encodenecessary=0;
  samplerate=22050;

  reinit=1;

  hbs=NULL;
  String dllname(api->path_getAppPath());
  dllname.cat(DIRCHARSTR);
  dllname.cat("wacs/lame_enc.dll");
  lamedll=LoadLibrary(dllname);
  if(lamedll) {
    beInitStream=(BEINITSTREAM)GetProcAddress(lamedll,TEXT_BEINITSTREAM);
    beCloseStream=(BECLOSESTREAM)GetProcAddress(lamedll,TEXT_BECLOSESTREAM);
    beEncodeChunk=(BEENCODECHUNK)GetProcAddress(lamedll,TEXT_BEENCODECHUNK);
  }
}

EncoderLame::~EncoderLame() {
  if(cont.gethWnd()) contparent->deleteChild(&cont);
  if(hbs) beCloseStream(hbs);
  if(lamedll) FreeLibrary(lamedll);
}

int EncoderLame::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  if(encodenecessary && !vbr) {
    // look for a mp3 chunk
    Chunk *mp3c=chunk_list->getChunk("MP3");
    if(mp3c) {
      int nch=mp3c->getInfo("nch");
      int br=mp3c->getInfo("bitrate");
      int srate=mp3c->getInfo("srate");
      int isvbr=mp3c->getInfo("vbr");
      // TODO> add sample rate checking
      if(!isvbr && br==bitrate && ((nch==2&&mode!=BE_MP3_MODE_MONO)||(nch==1&&mode==BE_MP3_MODE_MONO)) && srate==samplerate) {
        encodebuf.reset(); // so the next LAME chunk will be reset properly
        reinit=1;          // ditto
        return 1; // use this mp3 chunk
      }
    }
  }

  if(!lamedll) return 1;

  Chunk *c=chunk_list->getChunk("PCM");
  if(!c) return 1;

  int nch=c->getInfo("nch");
  int srate=c->getInfo("srate");
  int bps=c->getInfo("bps");
  char *pcmadr=(char *)c->getData();
  int bytes=c->getSize();

  if(reinit) {
    if(hbs) beCloseStream(hbs);
    hbs=NULL;
    
    BE_CONFIG bc={0,};
	  bc.dwConfig = BE_CONFIG_LAME;
	  bc.format.LHV1.dwStructVersion	= 1;
	  bc.format.LHV1.dwStructSize		= sizeof(bc);		
//	  bc.format.LHV1.dwSampleRate		= 44100;				// INPUT FREQUENCY
//	  bc.format.LHV1.dwReSampleRate		= 0;					// automatic resample
	  bc.format.LHV1.dwSampleRate		= 44100;				// INPUT FREQUENCY
	  bc.format.LHV1.dwReSampleRate		= samplerate;					// automatic resample
	  bc.format.LHV1.nMode				= mode;	// OUTPUT IN STREO
	  bc.format.LHV1.dwBitrate			= bitrate;					// MINIMUM BIT RATE
	  bc.format.LHV1.nPreset			= quality;		// QUALITY PRESET SETTING
    if(bitrate<32 || bitrate==144)	bc.format.LHV1.dwMpegVersion		= MPEG2;				// MPEG VERSION (I or II)
    else bc.format.LHV1.dwMpegVersion		= MPEG1;
	  bc.format.LHV1.dwPsyModel			= 0;					// USE DEFAULT PSYCHOACOUSTIC MODEL 
	  bc.format.LHV1.dwEmphasis			= 0;					// NO EMPHASIS TURNED ON
	  bc.format.LHV1.bOriginal			= TRUE;					// SET ORIGINAL FLAG

    if(vbr) {
  	  bc.format.LHV1.dwMaxBitrate		= vbrbr;					// MAXIMUM BIT RATE
    	bc.format.LHV1.bCRC				= TRUE;					// INSERT CRC
  	  bc.format.LHV1.bCopyright			= TRUE;					// SET COPYRIGHT FLAG	
  	  bc.format.LHV1.bPrivate			= TRUE;					// SET PRIVATE FLAG
  	  bc.format.LHV1.bWriteVBRHeader	= TRUE;					// YES, WRITE THE XING VBR HEADER
    	bc.format.LHV1.bEnableVBR			= TRUE;					// USE VBR
  	  bc.format.LHV1.nVBRQuality		= vbrquality;					// SET VBR QUALITY
    }

	  bc.format.LHV1.bNoRes				= TRUE;			

    lame_ss=0; lame_bs=0;
    beInitStream( &bc, &lame_ss, &lame_bs, &hbs);
    reinit=0;
  }

  if(!hbs) return 1;
  if(!lame_ss) {
    // remove mp3 chunk
    chunk_list->setChunk("MP3",NULL,0);
    return 1;
  }

  // resample to 44khz, 16bps, stereo
  {
    char *insamples=pcmadr;
    int size=0;
    if (nch == 2 && bps == 16) {
    // samples are correct size and channels. simple resample (working fine)
  		if (srate != 44100) {
			  int *in=(int*)insamples;
			  int r=0;
			  int dr=(srate<<12)/44100;
  
	  		int newlen=min(8192/4,MulDiv(bytes/4,44100,srate));
			  int x = newlen;
        size=newlen*4;
        buf.setSize(size);
			  int *out=(int*)buf.getMemory();
			  while (x--) {
				  *out++=in[r>>12];
				  r+=dr;
			  }
		  }
    } else { // resample and convert sample types
      if (srate == 44100 && bps == 16 && nch == 1) {
  			short *in=(short*)insamples;

	  		int newlen=min(8192/4,bytes/2);
			  int x = newlen;
        size=newlen*4;
        buf.setSize(size);
			  short *out=(short*)buf.getMemory();
			  while (x--) {
				  *out++=*in;
				  *out++=*in++;
			  }
      } else {
		    int r=0;
		    int dr=((srate<<12)/44100);

  		  int newlen=min(MulDiv((bytes / (bps/8)) / nch,44100,srate),8192/4);
        int x = newlen;

        size=newlen*4;
        buf.setSize(size);
		    short *out=(short *)buf.getMemory();

        if (nch == 1) {
		      while (x--) {
			      short spl;
			      if (bps == 8) {
				      int p=insamples[r>>12];
				      spl=(p^128)<<8;
			      } else spl=((short *)insamples)[r>>12];
			      *out++=spl;
			      *out++=spl;
			      r+=dr;
		      }
        } else {
		      while (x--) {
			      short spl,spl2;
			      if (bps == 8) {
				      int p=insamples[(r>>12)<<1];
				      spl=(p^128)<<8;
              p=insamples[((r>>12)<<1)+1];
              spl2=(p^128)<<8;
			      } else {
				      spl=((short *)insamples)[(r>>12)<<1];
              spl2=((short *)insamples)[((r>>12)<<1)+1];
			      }
			      *out++=spl;
			      *out++=spl2;
			      r+=dr;
		      }
        }
      }
	  }
    if(size) {
      bytes=size;
      pcmadr=buf.getMemory();
    }
  }
  
  // now that we're sure it's 44kbps,16bps,2ch, convert it to mono if we need to
  if(mode==BE_MP3_MODE_MONO) {
    short *insamples=(short *)pcmadr;
    int l=bytes/4;
    if(!buf.getSize()) buf.setSize(bytes/2);
    short *out=(short *)buf.getMemory();
    while(l--) {
      *out++=((*insamples++)+(*insamples++))>>1;
    }
    pcmadr=buf.getMemory();
    bytes/=2;
  }
  
  encodebuf.add(pcmadr,bytes);

  unsigned int req=lame_ss*2;
  MemBlock<char> send(req);
  MemBlock<char> receive(lame_bs);
  retmp3buf.reset();
  while((unsigned int)encodebuf.getSize()>=req) {
    encodebuf.get(send.getMemory(),req);
    DWORD s;
    beEncodeChunk(hbs,lame_ss,(PSHORT)send.getMemory(),(PBYTE)receive.getMemory(),&s);
    if(s) {
      retmp3buf.add(receive.getMemory(),s);
    }
  }
  chunk_list->setChunk("MP3",retmp3buf.getBuffer(),retmp3buf.getSize());
  
  return 1;
}

void EncoderLame::initConfig(ContWnd *configwnd, int x, int y) {
  if(!cont.gethWnd()) {
    cont.setDeleteChilds(0);
    cont.init(configwnd);
    cont.setVisible(0);
    BaseWnd *initwnd[]={&qualitytext,&brtext,&modetext,&vbrtext,&vbrbrtext,&vbrqualtext,&entext,&sratetext,
      &qualitydrop,&brdrop,&modedrop,&vbrbrdrop,&vbrqualdrop,&sratedrop,
      &vbrcheck,&encheck};
    for(int i=0;i<sizeof(initwnd)/sizeof(BaseWnd*);i++)
      initwnd[i]->init(&cont);

    cont.addChild(&qualitytext,0,2,55,16);
    qualitytext.setName("Quality");
    qualitytext.setTextSize(13);
    qualitytext.setCenter(1);
    cont.addChild(&qualitydrop,57,0,100,20);

    cont.addChild(&brtext,160,2,55,16);
    brtext.setName("Bitrate");
    brtext.setTextSize(13);
    brtext.setCenter(1);
    cont.addChild(&brdrop,210,0,50,20);

    cont.addChild(&modetext,0,24,55,16);
    modetext.setName("Mode");
    modetext.setTextSize(13);
    modetext.setCenter(1);
    cont.addChild(&modedrop,57,22,100,20);

    cont.addChild(&sratetext,160,24,45,16);
    sratetext.setName("Rate");
    sratetext.setTextSize(13);
    sratetext.setCenter(1);
    cont.addChild(&sratedrop,200,22,60,20);

    cont.addChild(&vbrcheck,0,50,12,12);
    cont.addChild(&vbrtext,14,48,30,16);
    vbrtext.setName("VBR");
    vbrtext.setTextSize(13);
    vbrtext.setCenter(0);

    cont.addChild(&vbrbrtext,0,70,55,16);
    vbrbrtext.setName("VBR Bitrate");
    vbrbrtext.setTextSize(13);
    vbrbrtext.setCenter(1);
    cont.addChild(&vbrbrdrop,57,68,50,20);
  
    cont.addChild(&vbrqualtext,110,70,70,16);
    vbrqualtext.setName("VBR Quality");
    vbrqualtext.setTextSize(13);
    vbrqualtext.setCenter(1);
    cont.addChild(&vbrqualdrop,185,68,75,20);

    cont.addChild(&encheck,0,94,12,12);
    cont.addChild(&entext,14,92,145,16);
    entext.setName("Encode only when necessary");
    entext.setTextSize(13);
    entext.setCenter(0);

    configwnd->addChild(&cont,x,y,500,500);
    configwnd->onResize();
    contparent=configwnd;
  }

  cont.setVisible(1);
}

void EncoderLame::removeConfig() {
  if(cont.gethWnd()) cont.setVisible(0);
}

static void bitrateDropSet(DropList *d, int b) {
  d->deleteAllItems();
  int br[]={8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128/*, 144*/, 160, 192, 224, 256 ,320};
  for(int i=0;i<sizeof(br)/sizeof(int);i++) {
    d->addItem(StringPrintf("%i",br[i]));
    if(br[i]==b) d->setCurSel(i);
  }
}

static int sratelist[]={0,8000,11025,12000,16000,22050,24000,32000,44100,48000};

void EncoderLame::updateConfig() {
  qualitydrop.deleteAllItems();
  qualitydrop.addItem("Normal");
  qualitydrop.addItem("Low");
  qualitydrop.addItem("High");
  qualitydrop.addItem("Voice");
  qualitydrop.addItem("R3mix");
  qualitydrop.addItem("Very High");
  qualitydrop.setCurSel(quality);

  bitrateDropSet(&brdrop, bitrate);

  modedrop.deleteAllItems();
/*  modedrop.addItem("Stereo");
  modedrop.addItem("Joint Stereo");
  modedrop.addItem("Dual Channel");
  modedrop.addItem("Mono");*/
  modedrop.addItem("Stereo");
  modedrop.addItem("Stereo (Discrete)");
  modedrop.addItem("Mono");
  int modetrans[]={1,0,1,2};
  modedrop.setCurSel(modetrans[mode]);

  vbrcheck.setChecked(vbr);

  bitrateDropSet(&vbrbrdrop, vbrbr);

  vbrqualdrop.deleteAllItems();
  vbrqualdrop.addItem("0 (Highest)");
  for(int i=1;i<9;i++) vbrqualdrop.addItem(StringPrintf("%i",i));
  vbrqualdrop.addItem("9 (Lowest)");
  vbrqualdrop.setCurSel(vbrquality);

  encheck.setChecked(encodenecessary);

  sratedrop.deleteAllItems();
  sratedrop.addItem("Auto");
  sratedrop.addItem("8khz");
  sratedrop.addItem("11khz");
  sratedrop.addItem("12khz");
  sratedrop.addItem("16khz");
  sratedrop.addItem("22khz");
  sratedrop.addItem("24khz");
  sratedrop.addItem("32khz");
  sratedrop.addItem("44khz");
  sratedrop.addItem("48khz");
  for(i=0;i<sizeof(sratelist)/sizeof(int);i++) if(sratelist[i]==samplerate) { sratedrop.setCurSel(i); break; }
}

void EncoderLame::getConfig() {
  quality=qualitydrop.getCurSel();
  char tmp[16];
  brdrop.getItemText(brdrop.getCurSel(),tmp,sizeof(tmp));
  bitrate=ATOI(tmp);
  int modetrans[]={BE_MP3_MODE_JSTEREO,BE_MP3_MODE_DUALCHANNEL,BE_MP3_MODE_MONO};
  mode=modetrans[modedrop.getCurSel()];
  vbr=vbrcheck.isChecked();
  vbrbrdrop.getItemText(vbrbrdrop.getCurSel(),tmp,sizeof(tmp));
  vbrbr=ATOI(tmp);
  vbrquality=vbrqualdrop.getCurSel();
  encodenecessary=encheck.isChecked();
  samplerate=sratelist[sratedrop.getCurSel()];
  reinit=1;
}

void EncoderLame::loadConfig(const char *pre) {
  quality=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s_lame_%s",pre,"quality"),quality);
  bitrate=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s_lame_%s",pre,"bitrate"),bitrate);
  mode=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s_lame_%s",pre,"mode"),mode);
  vbr=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s_lame_%s",pre,"vbr"),vbr);
  vbrbr=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s_lame_%s",pre,"vbrbr"),vbrbr);
  vbrquality=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s_lame_%s",pre,"vbrquality"),vbrquality);
  encodenecessary=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s_lame_%s",pre,"encodenecessary"),encodenecessary);
  samplerate=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s_lame_%s",pre,"samplerate"),samplerate);
}

void EncoderLame::saveConfig(const char *pre) {
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s_lame_%s",pre,"quality"),quality);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s_lame_%s",pre,"bitrate"),bitrate);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s_lame_%s",pre,"mode"),mode);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s_lame_%s",pre,"vbr"),vbr);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s_lame_%s",pre,"vbrbr"),vbrbr);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s_lame_%s",pre,"vbrquality"),vbrquality);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s_lame_%s",pre,"encodenecessary"),encodenecessary);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s_lame_%s",pre,"samplerate"),samplerate);
}
