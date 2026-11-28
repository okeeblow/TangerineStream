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
#include "wave.h"


Wave::Wave() {
  needseek=-1;
  fn=0;
  plr=0;
}

Wave::~Wave() {
	if (plr) plr->Release();
	if (fn) free(fn);
}

int Wave::getInfos(MediaInfo *infos) {
  m_rd.r=infos->getReader();
  if(!m_rd.r) return 0;
  bool ks=0;
  if (open(infos->getFilename(),&ks)) return 0;

  WAVEINFO wi;

  infos->setLength(plr->GetLengthMS());
  infos->setTitle(Std::filename(infos->getFilename()));
  plr->GetInfo(&wi);
  
  //infos->setInfo(wi.desc);
/*  char _ch[16];
  char _khz[16];
  char * ch;
  if (wi.fmt.nch==1) ch="mono";
  else if (wi.fmt.nch==2) ch="stereo";
  else {ch=_ch;sprintf(_ch,"%ichannels",wi.fmt.nch);}
  
  if ((wi.fmt.sr/100)%10)
	  sprintf(_khz,"%i.%i",wi.fmt.sr/1000,(wi.fmt.sr/100)%10);
  else sprintf(_khz,"%i",wi.fmt.sr/1000);

  infos->setInfo(StringPrintf("%ikbps %s %skHz", wi.kbps, ch,_khz));*/
  infos->setInfo(wi.desc);
  
  return 0;
}

int Wave::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  m_rd.r=infos->getReader();
  if(!m_rd.r) return 0;
  if (open(infos->getFilename(),killswitch)) return 0;


  if(needseek!=-1) {
	plr->SeekMS(needseek,0,killswitch);
	needseek=-1;
  }


  return plr->processData(infos,chunk_list,samplebuffer,sizeof(samplebuffer),killswitch);
}

int Wave::corecb_onSeeked(int newpos) {
	if (!plr->CanSeek()) return 0;
  needseek=newpos;
  return 0;
}




/*
void get_temp_file(char* fn)
{
	static char tmp_path[MAX_PATH];
	if (!tmp_path[0]) GetTempPath(MAX_PATH,tmp_path);
	static DWORD num;
	if (num==0) num=GetTickCount();
	wsprintf(fn,"%sdm%x.tmp",tmp_path,num++);
}

void file2title(char* f,char* t)
{
	char* p=strrchr(f,'\\');
	if (!p)
	{
		p=strrchr(f,'/');
		if (!p) p=f;
		else p++;
	}
	else p++;
	strcpy(t,p);
	p=strrchr(t,'.');
	if (p) *p=0;
}
*/

char* exts[N_EXTS]={"WAV","VOC","AU","SND","AIF","AIFF","AIFC","IFF","SVX","AUD"};

static char d_wav[]="Windows waveform";
static char d_voc[]="Creative VOC";
static char d_au[]="Next/Sun";
static char d_aiff[]="Apple AIFF";
static char d_svx[]="Amiga 8SVX";
static char d_aud[]="Westwood AUD format";

char* ext_descs[N_EXTS]={d_wav,d_voc,d_au,d_au,d_aiff,d_aiff,d_aiff,d_svx,d_svx,d_aud};
char is_default[N_EXTS]={1,1,1,1,1,1,0,0,0,0};


UINT get_def_exts()
{
	UINT ret=0;
	UINT n;
	for(n=0;n<N_EXTS;n++) if (is_default[n]) ret|=1<<n;
	return ret;
}

#define FMT_WAV 1
#define FMT_VOC 2
#define FMT_AU 3
#define FMT_AIFF 4
#define FMT_AUD 5
#define FMT_ALAW 6
#define FMT_ULAW 7

int test_ext(const char* url)
{
	char* ext=strrchr(url,'.');
	if (!ext) return -1;
	ext++;
	int n;
	for(n=0;n<N_EXTS;n++)
	{
		if ((cfg_ext_mask&(1<<n)) && !stricmp(ext,exts[n])) return n;
	}
	return -1;
}

int get_fmt(const char* url,WReader* r,bool * ks)
{
	int e=test_ext(url);

	DWORD dw=0;
	r->Read((char*)&dw,4,ks);
	r->Seek(0,ks);
	if (e==9 && (dw&0xFFFF)==22050) return FMT_AUD;
	switch(dw)
	{
	case _rv('RIFF'):
		return FMT_WAV;
	case _rv('Crea'):
		return FMT_VOC;
	case _rv('.snd'):
		return FMT_AU;
	case _rv('FORM'):
		return FMT_AIFF;
	default:
		return 0;
	}
};

HINSTANCE hIns=0;
/*
int CWavSource::GetPosition(void)
{
	if (plr)
	{
		return byte2ms(pos_bytes,&plr->fmt);
	}
	else return 0;
}

int CWavSource::SetPosition(int p)
{
	if (plr && plr->CanSeek())
	{
		bool ks=0;
		plr->SeekMS(p,&pos_bytes,&ks);
		return 0;
	} else return 1;
}
*/
CWavPlayer * CLoader::run(int type)
{
	error=0;
	CWavPlayer* p=0;
	switch(type)
	{
	case FMT_AUD:
		p=aud_create();
		break;
	case FMT_WAV: //WAV
		p=wav_create();
		break;
	case FMT_VOC: //VOC
		p=voc_create();
		break;
	case FMT_AU:
		p=au_create();
		break;
	case FMT_AIFF:
		p=aiff_create();
		break;
	}
	if (*ks)
	{
		if (p) p->Release();
		p=0;
	}
#if 0
	if (p && !info && !p->useswa3shiz())//can't do shit if player has its own processdata implementation
	{
		if (cfg_8216 && p->fmt.bps==8)
		{
			CWavPlayer* cvt=cvt16_create(p);
			if (cvt) p=cvt;
		}
		else if (cfg_downcvt && p->fmt.bps>16)
		{
			CWavPlayer* cvt=cvt16_create(p);
			if (cvt) p=cvt;
		}
		if (cfg_mono && p->fmt.nch>1)
		{
			CWavPlayer* cvt=mono_create(p);
			if (cvt) p=cvt;
		}
	}
#endif
	return p;
}


CWavPlayer* plr_create(int type,WReader* r,bool info,bool * ks)
{
	CLoader l;
	l.ks=ks;
	l.r=r;
	l.info=info;
	return l.run(type);
}

int Wave::open(const char *url,bool * ks)
{
	if (plr && fn && !stricmp(url,fn)) return 0;
	if (plr) {plr->Release();plr=0;}
	if (fn) {free(fn);fn=0;}
	WReader * r=&m_rd;

	{
		WReader* s=seekabilize(r);
		if (!s)
		{
			r->Release();
			return 1;
		}
		r=s;
	}
	int type=get_fmt(url,r,ks);
	if (!type)
	{
		r->Release();
		//if (!*ks) m_player->Error("Invalid file type.");
		return 1;
	}

	plr=plr_create(type,r,0,ks);

	if (!plr)
	{
//		if (!*ks) m_player->Error("Unable to start playback. Missing codec or unsupported format.");
		return 1;
	}

	fn=strdup(url);

	return 0;
}

/*
class CWavInfo : public WInputInfo
{
	CWavPlayer* plr;
	char fn[MAX_PATH];
	virtual int  Open(char *url);
	virtual void GetTitle(char *buf, int maxlen);
	virtual void GetInfoString(char *buf, int maxlen);
	virtual int  GetLength(void);
	virtual int  GetMetaData(char *name, char *data, int data_len) { if (data&&data_len>0) *data=0; return 0; }
	virtual void Release() {if (plr) plr->Release();delete this;}
};

int CWavInfo::GetLength()
{
	if (plr) return plr->GetLengthMS();
	else return -1;
}

void CWavInfo::GetInfoString(char *buf, int maxlen)
{
	if (plr)
	{
		WAVEINFO i;
		plr->GetInfo(&i);
		strncpy(buf,i.desc,maxlen);
	}
	else buf[0]=0;
}

void CWavInfo::GetTitle(char *buf, int maxlen)
{
	file2title(fn,buf);
};

int CWavInfo::Open(char* url)
{
	if (plr) {plr->Release();plr=0;}
	strncpy(fn,url,MAX_PATH);
	WReader* r=m_info->GetReader(url);
	if (!r) return 1;
	bool ks=0;
	if (r->Open(url,&ks))
	{
		r->Release();
		return 1;
	}
	int type=get_fmt(url,r,&ks);
	if (!type)
	{
		r->Release();
		return 1;
	}

	plr=plr_create(type,r,1,&ks);

	return plr ? 0 : 1;
}
*/
void config_reset();

void PLUGIN_INIT(int s,HINSTANCE _hi)
{
	static int state;
	if (state!=s)
	{
		if (state=s)
		{
			hIns=_hi;
			config_reset();

//			do_cfg(0);
//			build_fmtstring();

		}
		else
		{
//			do_cfg(1);
		}
	}
}

int Wave::canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
  if(name)
  {
	  return test_ext(name)!=-1;

  }
    return 0;
}

BOOL APIENTRY DllMain(HANDLE hMod,DWORD r,void*)
{
    if (r==DLL_PROCESS_ATTACH)
	{
		PLUGIN_INIT(1,(HINSTANCE)hMod);
	}
	else if (r==DLL_PROCESS_DETACH)
	{
		PLUGIN_INIT(0,0);		
	}
	return 1;
}
int CWrapReader::Read(char *buffer, UINT length, bool *killswitch)
{
	if (!r) return 0;
	return r->read(buffer,length);
}

int CWrapReader::GetLength(void)
{
	return r->getLength();
}

int CWrapReader::CanSeek(void)
{
	return r->canSeek();
}

int CWrapReader::Seek(UINT position, bool *killswitch)
{
	return r->seek(position);
}