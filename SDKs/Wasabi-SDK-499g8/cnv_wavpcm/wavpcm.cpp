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
#include "wavpcm.h"

int WavPcm::getInfos(MediaInfo *infos) {
  // TODO> send command to say to close the reader

	MMCKINFO    mmckinfoParent;     // parent chunk information 
	MMCKINFO    mmckinfoSubchunk;   // subchunk information structure 
	DWORD       dwFmtSize;          // size of "FMT" chunk
	DWORD       dwDataSize;         // size of "DATA" chunk 
	WAVEFORMATEX  *pFormat;           // address of "FMT" chunk 
	HMMIO hmmio;
  char url[WA_MAX_PATH];
  STRCPY(url,infos->getFilename());

	hmmio=mmioOpen(url,NULL,MMIO_READ|MMIO_ALLOCBUF);
	if (!hmmio) 
  {
    infos->setInfo("file not found");
    return 0;
  }
	mmckinfoParent.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 
	if (mmioDescend(hmmio, (LPMMCKINFO) &mmckinfoParent, NULL, MMIO_FINDRIFF)) 
	{ 
    infos->setInfo("This is not a waveform-audio file.");
		mmioClose(hmmio, 0); 
		return 0;
	} 
	mmckinfoSubchunk.ckid = mmioFOURCC('f', 'm', 't', ' '); 
	if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)) 
	{ 
    infos->setInfo("Waveform-audio file has no \"FMT\" chunk.");
	  mmioClose(hmmio, 0); 
		return 0; 
  } 
	dwFmtSize = mmckinfoSubchunk.cksize;
	pFormat = (WAVEFORMATEX *) malloc(dwFmtSize);		
	if (mmioRead(hmmio, (HPSTR) pFormat, dwFmtSize) != (int)dwFmtSize) { 
    infos->setInfo("Failed to read format chunk."); 
		mmioClose(hmmio, 0); 
		free(pFormat);
		return 0;
	}
	mmioAscend(hmmio, &mmckinfoSubchunk, 0); 
	mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a'); 
	if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent,MMIO_FINDCHUNK)) 
	{ 
    infos->setInfo("Waveform-audio file has no data chunk."); 
		free(pFormat);
	  mmioClose(hmmio, 0); 
	  return 0; 
	} 
  dwDataSize = mmckinfoSubchunk.cksize; 
	if (dwDataSize == 0L)
	{ 
    infos->setInfo("The data chunk contains no data."); 
	  free(pFormat);
		mmioClose(hmmio, 0); 
	  return 0; 
  }
	int lengthinms = (dwDataSize/pFormat->nAvgBytesPerSec)*1000;
	{
		int l=lengthinms%pFormat->nAvgBytesPerSec;
		lengthinms += (l*1000)/pFormat->nAvgBytesPerSec;
	}
  infos->setInfo(StringPrintf("%d bit %s waveform at %d.%dkHz",pFormat->wBitsPerSample,pFormat->nChannels==2?"stereo":"mono",pFormat->nSamplesPerSec/1000,(pFormat->nSamplesPerSec/100)%10));
  infos->setLength(lengthinms);

	free(pFormat);

  mmioClose(hmmio,0);

  return 0;
}

int WavPcm::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  if(!m_opened) {
  	MMCKINFO    mmckinfoParent;     // parent chunk information 
  	MMCKINFO    mmckinfoSubchunk;   // subchunk information structure 
  	DWORD       dwFmtSize;          // size of "FMT" chunk
  	DWORD       dwDataSize;         // size of "DATA" chunk 
  	WAVEFORMATEX  *pFormat;           // address of "FMT" chunk 
  	HMMIO hmmio;
    bytes_in_sbuf=bytes_in_rbuf=0;

	  hmmio=mmioOpen((char *)infos->getFilename(),NULL,MMIO_READ|MMIO_ALLOCBUF);
	  if (!hmmio) {
      infos->warning("File not found."); 
      return -1;
    }
	  mmckinfoParent.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 
	  if (mmioDescend(hmmio, (LPMMCKINFO) &mmckinfoParent, NULL, MMIO_FINDRIFF))  { 
      infos->warning("This is not a waveform-audio file."); 
		  mmioClose(hmmio, 0); 
		  return -1;
	  } 
	  mmckinfoSubchunk.ckid = mmioFOURCC('f', 'm', 't', ' '); 
	  if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)) { 
      infos->warning("Waveform-audio file has no \"FMT\" chunk."); 
	    mmioClose(hmmio, 0); 
		  return 1; 
    } 
	  dwFmtSize = mmckinfoSubchunk.cksize;
	  pFormat = (WAVEFORMATEX *) malloc(dwFmtSize);		
	  if (mmioRead(hmmio, (HPSTR) pFormat, dwFmtSize) != (int)dwFmtSize) { 
      infos->warning("Failed to read format chunk."); 
		  mmioClose(hmmio, 0); 
		  free(pFormat);
		  return 1;
	  }
  	mmioAscend(hmmio, &mmckinfoSubchunk, 0); 
  	mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a'); 
  	if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent,MMIO_FINDCHUNK)) { 
      infos->warning("Waveform-audio file has no data chunk."); 
		  free(pFormat);
	    mmioClose(hmmio, 0); 
	    return 1; 
	  } 
    dwDataSize = mmckinfoSubchunk.cksize; 
	  if (dwDataSize == 0L) { 
      infos->warning("The data chunk contains no data."); 
	    free(pFormat);
		  mmioClose(hmmio, 0); 
	    return 1; 
    }
	  start_in_wav_file=mmioSeek(hmmio,0,SEEK_CUR);
	  lengthinms = (dwDataSize/pFormat->nAvgBytesPerSec)*1000;
	  {
  		int l=lengthinms%pFormat->nAvgBytesPerSec;
		  lengthinms += (l*1000)/pFormat->nAvgBytesPerSec;
	  }
	  blockalign = pFormat->nBlockAlign;
	  bps = pFormat->nAvgBytesPerSec;
	  lengthinbytes=dwDataSize;
	  posinbytes=0;
	  if (pFormat->wFormatTag==WAVE_FORMAT_PCM) use_acm=0;
	  else use_acm=1;
	  if (use_acm)
	  {
  		memset(&fmt,0,sizeof(fmt));
		  fmt.wFormatTag=WAVE_FORMAT_PCM;
		  if (acmFormatSuggest(NULL,pFormat,&fmt,sizeof(fmt),ACM_FORMATSUGGESTF_WFORMATTAG))
		  {
        infos->warning("Could not find ACM destination format."); 
			  mmioClose(hmmio,0);
			  free(pFormat);
			  return 1;
		  }
		  output_bsize=1024;
		  if (fmt.nChannels==1)
  			output_bsize/=2;
		  if (fmt.wBitsPerSample==8)
  			output_bsize/=2;
		  {
  			int z;
			  if ((z=acmStreamOpen(&has,NULL,pFormat,&fmt,NULL,0,0,0)))
			  {
				  infos->warning(StringPrintf("Couldn't open ACM stream: %d",z));
				  mmioClose(hmmio,0);
				  free(pFormat);
				  has=0;			
				  return 1;
			  }
		  }
		  if (acmStreamSize(has,output_bsize,(unsigned long *)&s_inputbuf,ACM_STREAMSIZEF_DESTINATION))
		  {
        infos->warning("Couldn't find ACM stream buffer size"); 
			  mmioClose(hmmio,0);
			  acmStreamClose(has,0);
			  free(pFormat);
			  has=0;			
			  return 1;
		  }
		  memset(&ash,0,sizeof(ash));
		  ash.cbStruct=sizeof(ash);
		  ash.pbSrc=(unsigned char *) malloc(s_inputbuf);
		  ash.cbSrcLength=s_inputbuf;
		  ash.pbDst = (unsigned char *) malloc(output_bsize);
		  ash.cbDstLength=output_bsize;
		  acmStreamPrepareHeader(has,&ash,0);
		  ash.cbSrcLengthUsed=0;
		  ash.cbDstLengthUsed=0;
	    is8bit = fmt.wBitsPerSample==8?1:0;
	    ismono = fmt.nChannels==1?1:0;
	  }
	  else
	  {
  		memcpy(&fmt,pFormat,min(dwFmtSize,sizeof(fmt)));
		  output_bsize=1024;
		  if (fmt.nChannels==1)
  			output_bsize/=2;
		  if (fmt.wBitsPerSample==8)
  			output_bsize/=2;
  	}
  	wav_file=hmmio;
  	free(pFormat);
    m_opened=true;
  }                                                       
  

  int btotal=0;
  int bps=fmt.wBitsPerSample;
  int nch=fmt.nChannels;
  int srate=fmt.nSamplesPerSec;
  int bytes=4096;
  char *sample_buffer=m_buffer;

  while (bytes > 0)
  {
    if (ateof && bytes_in_sbuf == 0)
    {
      break;
//      return btotal;
    }
	  
    while (bytes_in_sbuf < output_bsize) 
	  {
		  int o_len=use_acm?ash.cbSrcLength:output_bsize;
		  if (bytes_in_rbuf < o_len)
		  {
			  int r_len = o_len-bytes_in_rbuf;
			  if (r_len + posinbytes >= lengthinbytes)
				  r_len = lengthinbytes-posinbytes;
			  if (use_acm) r_len = mmioRead(wav_file,(char *)ash.pbSrc+bytes_in_rbuf,r_len);
			  else r_len = mmioRead(wav_file,sbuf+bytes_in_sbuf,r_len);
			  if (r_len <= 0) break;
  		  posinbytes += r_len;
			  if (use_acm) bytes_in_rbuf += r_len;
			  else bytes_in_sbuf += r_len;
		  }
		  if (use_acm)
		  {
			  ash.cbSrcLength = bytes_in_rbuf;
			  acmStreamConvert(has,&ash,ACM_STREAMCONVERTF_BLOCKALIGN);	
			  memcpy(sbuf+bytes_in_sbuf,ash.pbDst,ash.cbDstLengthUsed);
			  memcpy(ash.pbSrc,ash.pbSrc+ash.cbSrcLengthUsed,
	      		  bytes_in_rbuf-ash.cbSrcLengthUsed);
			  bytes_in_sbuf += ash.cbDstLengthUsed;
			  bytes_in_rbuf -= ash.cbSrcLengthUsed;
			  ash.cbSrcLength=o_len;
		  }
	  }
  	if (bytes_in_sbuf > 0)
		{
			int l;
			if (bytes_in_sbuf < output_bsize)
			{
				ateof=1;
			}
			l = min(bytes_in_sbuf,bytes);
      memcpy(sample_buffer,sbuf,l);
      sample_buffer+=l;
      btotal+=l;
      bytes_in_sbuf-=l;
      bytes-=l;
			if (bytes_in_sbuf <= 0) bytes_in_sbuf = 0;
			else memcpy(sbuf,sbuf+output_bsize,bytes_in_sbuf);
    }
    else ateof=1;
  }
  if(btotal) {
    ChunkInfosI *cinfos=new ChunkInfosI();
    cinfos->addInfo("bps",bps);
    cinfos->addInfo("srate",srate);
    cinfos->addInfo("nch",nch);
    chunk_list->setChunk("PCM", m_buffer, btotal, cinfos);
    return 1;
  }
  return 0;
}

int WavPcm::corecb_onSeeked(int newpos) { 
  if(!m_opened) return 0;
  int a=newpos;
  bytes_in_sbuf=0;
  bytes_in_rbuf=0;
  posinbytes = (a/1000)*bps;
  posinbytes += ((a%1000)*bps)/1000;
  posinbytes -= posinbytes % blockalign;
  if (use_acm)
  {
	  ash.cbSrcLengthUsed=0;
	  ash.cbDstLengthUsed=0;
  }
  mmioSeek(wav_file,posinbytes+start_in_wav_file,SEEK_SET);
  ateof=false;
  return 0; 
}

