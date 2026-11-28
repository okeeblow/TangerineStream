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
//#define STRICT
//#include <windows.h>
//#include "core_api.h"
#include "main.h"
//#include "in2.h"

#ifdef _DEBUG
#include <malloc.h>
#endif

#define IPC_GETHTTPGETTER 240

typedef int (*t_getf)(HWND hwnd, char *url, char *file, char *dlgtitle);

T_RF_create RF_create=0;
T_gzip_writefile gzip_writefile=0;

static bool paused;
static int volume=255;
static int pan;
static char cur_file[MAX_PATH];
static WInputSource *source;
static input_source *desc;
static HANDLE thread;
static char *sample_buf;

static UINT hack_kbps()
{
	UINT r=0;
	if (source)
	{
		CWavPlayer* plr=(CWavPlayer*)((DWORD*)source)[2];
		if (plr)
		{
			WAVEINFO i;
			plr->GetInfo(&i);
			r=i.kbps;
		}
	}
	return r;
}

#define SUPPORT_READERS


#ifdef SUPPORT_READERS
#define MAX_READERS 16
static UINT n_readers;
static struct
{
	HINSTANCE dll;
	reader_source *r;
} readers[MAX_READERS];
#endif //SUPPORT_READERS

void Config(HWND);
void About(HWND);

extern "C" {int getInputSource(HINSTANCE hDllInstance, input_source **inStruct);}

extern input_source theSrc;

class CMemReader : public WReader
{
public:
	BYTE* mem;
	UINT sz;
	UINT pos;
	char *GetDescription() { return ""; };
	int Open(char *url, bool *killswitch);
	int Read(char *buffer, int length, bool *killswitch) {if (!mem) return 0;if (length+pos>sz) length=sz-pos;memcpy(buffer,mem+pos,length);pos+=length;return length;}
	int GetLength(void) {return sz;} 
	int CanSeek(void) {return 1;};
	int Seek(int position, bool *killswitch) {pos=position;return 0;};
	void Release(int) {if (mem) free(mem);delete this;}
	CMemReader() {mem=0;sz=0;pos=0;}

};

bool Download(char* url,UINT* f_size,BYTE** m_buf)
{
	typedef int (*t_getf)(HWND hwnd, char *url, char *file, char *dlgtitle);

	t_getf getf;

	int t=SendMessage(mod.hMainWindow,WM_USER,0,IPC_GETHTTPGETTER);
	if (!t || t==1)
	{
		MessageBox(mod.hMainWindow,"URLs only supported in Winamp 2.50+",ERROR,MB_ICONERROR);
		return 0;
	}
	else
	{
		int rv=0;
		char tmp[MAX_PATH];
		get_temp_file(tmp);
		HANDLE f;
		DWORD br,s;
		void* b;
		getf=(t_getf)t;
		if (getf(mod.hMainWindow,url,tmp,"Retrieving file")) goto fail;
		f=CreateFile(tmp,GENERIC_READ,0,0,OPEN_EXISTING,0,0);
		if (f==INVALID_HANDLE_VALUE) goto fail;
		br=0;
		s=GetFileSize(f,0);
		if (!s) goto fail;
		b=malloc(s);
		if (!b) goto fail;
		ReadFile(f,b,s,&br,0);
		rv=1;
		*f_size=br;
		*m_buf=(BYTE*)b;
fail:
		CloseHandle(f);
		DeleteFile(tmp);
		return rv;
	}
}


int CMemReader::Open(char* url,bool*)
{
	sz=pos=0;
	if (mem) {free(mem);mem=0;}
	HANDLE f=CreateFile(url,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
	if (f!=INVALID_HANDLE_VALUE)
	{
		sz=GetFileSize(f,0);
		mem=(BYTE*)malloc(sz);
		if (!mem) {CloseHandle(f);return 1;}
		ReadFile(f,mem,sz,(DWORD*)&sz,0);
		CloseHandle(f);
		return 0;
	}
	return !Download(url,&sz,&mem);
}
WReader* get_reader(char*);

static WReader* _fastcall cb_getreader(int,int,char* p) {return get_reader(p);}

static void* icb_vtbl[]={cb_getreader};
static void* theIcb=&icb_vtbl;

static void _fastcall cb_error(int,int,char *reason) {MessageBox(mod.hMainWindow,reason,NAME " error",MB_ICONERROR);}
static void _fastcall cb_warning(int,int,char *warning) {MessageBox(mod.hMainWindow,warning,NAME " warning",MB_ICONWARNING);}
static void _fastcall cb_nothing(int,int,char*) {};
static void _fastcall cb_nothing2(int,int,char*,int) {};

static void* pcb_vtbl[]={cb_getreader,cb_error,cb_warning,cb_nothing,cb_nothing,cb_nothing2,cb_nothing};
static void* thePcb=&pcb_vtbl;


class CFileReader : public WReader
{
  public:
	HANDLE f;
	CFileReader() {f=0;};
	char *GetDescription() { return ""; };
	int Open(char *url, bool *killswitch) {f=CreateFile(url,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);return f==INVALID_HANDLE_VALUE;}
	int Read(char *buffer, int length, bool *killswitch) {DWORD br=0;ReadFile(f,buffer,length,&br,0);return br;}
	int GetLength(void) {return GetFileSize(f,0);} 
	int CanSeek(void) {return 1;};
	int Seek(int position, bool *killswitch) {SetFilePointer(f,position,0,FILE_BEGIN);return 0;}
	void Release(int) {CloseHandle(f);delete this;}
};

WReader *get_reader(char* url)
{
	WReader* ret=0;
#ifdef SUPPORT_READERS
	UINT n;
	for(n=0;n<n_readers;n++)
	{
		if (readers[n].r)
		{
			if (readers[n].r->ismine(url)) {ret=readers[n].r->create();break;}
		}
	}
	if (ret)
	{
		ret->m_player=(WPlayer_callback*)&thePcb;
		return ret;
	}
#endif
	if (strncmp(url,"http:",5)==0 || strncmp(url,"ftp:",4)==0 || strncmp(url,"https:",6)==0) return _new(CMemReader);
	return _new(CFileReader);
}

typedef int (*readerfunc)(HINSTANCE,reader_source**);

void PLUGIN_INIT(int s,HINSTANCE _hi);

void Init()
{
//	getInputSource(mod.hDllInstance,&desc);
	desc=&theSrc;
	mod.description=desc->description;
/*	strcpy(extensions,desc->extension_list);
	strcat(extensions,";");
	UINT n,m=strlen(extensions);
	for(n=0;n<m;n++)
	{
		if (extensions[n]==';') extensions[n]=0;
	}*/
	PLUGIN_INIT(1,mod.hDllInstance);
	log_write("PLUGIN_INIT returned");
#ifdef SUPPORT_READERS
	log_write("scanning for read_*.dll");
	char path[MAX_PATH];
	GetModuleFileName(mod.hDllInstance,path,MAX_PATH);
	char* p=strrchr(path,'\\');
	if (p) p++;
	else p=path;
	strcpy(p,"read_*.dll");
	WIN32_FIND_DATA fd;
	HANDLE f=FindFirstFile(path,&fd);
	if (f!=INVALID_HANDLE_VALUE)
	{
		do
		{
			strcpy(p,fd.cFileName);
			log_write("attempting to load dll:");
			log_write(path);
			HINSTANCE dll=LoadLibrary(path);
			if (dll)
			{
				readerfunc r=(readerfunc)GetProcAddress(dll,"readerSource");
				if (!r) {FreeLibrary(dll);continue;}
				readers[n_readers].dll=dll;
				r(dll,&readers[n_readers++].r);
				log_write("dll loaded OK");
				if (!stricmp(fd.cFileName,"read_file.dll"))
				{
					RF_create=(T_RF_create)GetProcAddress(dll,"RF_create");
					gzip_writefile=(T_gzip_writefile)GetProcAddress(dll,"gzip_writefile");
				}
			}
#ifdef USE_LOG
			else log_write("can't load dll");
#endif
		} while(FindNextFile(f,&fd));
		FindClose(f);
	}
#endif	//SUPPORT_READERS
}

void Quit()
{
#ifdef SUPPORT_READERS
	UINT n;
	for(n=0;n<n_readers;n++)
	{
		FreeLibrary(readers[n].dll);
	}
	n_readers=0;
#endif
	PLUGIN_INIT(0,0);
}

void file2title(char* f,char* t);

void GetFileInfo(char *file, char *title, int *len)
{
	bool use_src=0;
	if (!file || !*file)
	{
		file=cur_file;
		if (source) use_src=1;
	}
	if (source && !stricmp(file,cur_file)) use_src=1;
	if (title)
	{
		file2title(file,title);
	}
	if (len)
	{
		*len=0;
		if (use_src)
		{
			*len=source->GetLength();
		}
		else
		{
			WInputInfo *i=desc->createinfo();
			if (!i) return;
			i->m_info=(WInfo_callback*)&theIcb;
			if (!i->Open(file))
			{
				/*if (len) */*len=i->GetLength();
		//		if (title) i->GetTitle(title,256);
			}
			i->Release(0);
		}
	}
}


int infoDlg(char *fn, HWND hwnd)
{
	char tx[512];
	tx[0]=0;
	if (!stricmp(fn,cur_file) && source)
	{
		source->GetInfoString(tx,512);
	}
	else
	{
		WInputInfo* i=desc->createinfo();
		if (!i) return 1;
		i->m_info=(WInfo_callback*)&theIcb;
		if (i->Open(fn)) {i->Release(0);return 1;}
		
		i->GetInfoString(tx,512);
		i->Release(0);
	}
	if (!tx[0]) return 1;
	MessageBox(hwnd,tx,"file info",0);
	return 0;
}

int InfoBox(char *file, HWND parent)
{
	if (!file) file=cur_file;
	return infoDlg(file,parent);
}

int IsOurFile(char *fn)
{
	return desc->ismine(fn);
}

static bool kill;
static int pos_ms;
static int seek_to;

DWORD WINAPI PlayThread(void*)
{
	int sr,bps,nch;
	short * visbuf;
	pos_ms=0;
	seek_to=-1;
	int pos_base=0;
	int samp_wr=0;
	source->GetSamples(0,0,&bps,&nch,&sr,&kill);
	int s_size=576 * (bps>>3) * nch;
	int done=0;
	if (bps>16)
	{
		visbuf=(short*)alloca(576*2*nch);
	}
	else visbuf=0;

	while(!kill)
	{
		if (seek_to!= -1)
		{
			if (!source->SetPosition(seek_to))
			{
				pos_base=pos_ms=seek_to;
				mod.outMod->Flush(pos_ms);
				samp_wr=0;
				seek_to=-1;
				done=0;
			}
		}
		if (done)
		{
//			mod.outMod->CanWrite();
			if (!mod.outMod->IsPlaying())
			{
				PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
				return 0;
			}
			Sleep(30);
		}
		else if (mod.outMod->CanWrite() >= ((576*nch*(bps>>3))<<(mod.dsp_isactive()?1:0)))
		{	
			int l=source->GetSamples(sample_buf,s_size,&bps,&nch,&sr,&kill);
			if (l<=0)
			{
				mod.outMod->Write(sample_buf,0);
				done=1;
			}
			else
			{
				char * vis=sample_buf;
				UINT vis_bps=bps;
				if (bps>16)
				{
					UINT n;
					UINT d=bps>>3;
					char * foo=sample_buf+d-2;
					for(n=0;n<576*nch;n++)
					{
						visbuf[n]=*(short*)foo;
						foo+=d;
					}
					vis=(char*)visbuf;
					vis_bps=16;
				}
				mod.SAAddPCMData(vis,nch,vis_bps,pos_ms);
				mod.VSAAddPCMData(vis,nch,vis_bps,pos_ms);

				if (mod.dsp_isactive())
				{
					l=(l<<3)/(bps*nch);
					l=mod.dsp_dosamples((short*)sample_buf,l,bps,nch,sr);
					l*=(nch*bps)>>3;
				}
				if (kill) break;
				mod.outMod->Write((char*)sample_buf,l);

				samp_wr+=(8*l)/(bps*nch);
				pos_ms=pos_base+MulDiv(1000,samp_wr,sr);
			}
		}
		else Sleep(20);
	}

	return 0;
}

static void src_del()
{
	WInputSource* i=(WInputSource*)InterlockedExchange((long*)&source,0);
	if (i)
	{
		i->Release(0);
	}
}

DWORD WINAPI FooThread(void*)
{
	UINT n=5;
	for(;n&&!kill;n--) Sleep(100);
	if (!kill) PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
	return 0;
}

static int out_open;

int Play(char *fn)
{
	paused=0;
	kill=0;
	source=desc->create();
	if (!source) return 1;
	source->m_player=(WPlayer_callback*)&thePcb;
	int rv=source->Open(fn,&kill);
	if (rv)
	{
		src_del();
		if (rv==2)
		{
			DWORD id;
			thread=CreateThread(0,0,FooThread,0,0,&id);
			return 0;
		}
		else return 1;
	}
	strcpy(cur_file,fn);

	int sr,bps,nch;
	source->GetSamples(0,0,&bps,&nch,&sr,&kill);
	mod.SetInfo(hack_kbps(),sr/1000,nch,1);
	sample_buf=(char*)malloc(576*(bps>>3)*nch*2);
	if (!sample_buf)
	{
		src_del();
		return 1;
	}

	
	int max_l=mod.outMod->Open(sr,nch,bps,-1,-1);
	if (max_l<0)
	{
		free(sample_buf);
		src_del();
		return 1;
	}
	mod.outMod->SetVolume(-666);
	mod.outMod->SetPan(pan);
	mod.SAVSAInit(max_l,sr);
	mod.VSASetInfo(sr,nch);

	out_open=1;

	DWORD id;
	thread=CreateThread(0,0,PlayThread,0,CREATE_SUSPENDED,&id);
	SetThreadPriority(thread,priority_tab[cfg_priority]);
	ResumeThread(thread);
	
	return 0;
}

void Pause()
{
	if (source && !paused)
	{
		source->Pause(paused=1);
		mod.outMod->Pause(1);
	}
}

void UnPause()
{
	if (source && paused)
	{
		source->Pause(paused=0);
		mod.outMod->Pause(0);
	}
}

int IsPaused()
{
	return paused;
}

void Stop()
{
	if (thread)
	{
		kill=1;
		if (WaitForSingleObject(thread,4000)!=WAIT_OBJECT_0)
		{
			TerminateThread(thread,0);
			MessageBox(mod.hMainWindow,"error asking thread to die",0,MB_ICONERROR);
		}
		CloseHandle(thread);
		thread=0;
	}
	if (out_open)
	{
		mod.outMod->Close();
		mod.SAVSADeInit();
		out_open=0;
	}
	src_del();
	if (sample_buf)
	{
		free(sample_buf);
		sample_buf=0;
	}
}

void EQSet(int on, char data[10], int preamp)
{
}

int GetLength()
{
	if (source) return source->GetLength();
	else return -1;
}

int GetOutputTime()
{
	return pos_ms+(mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime()); 
}

void SetOutputTime(int t)
{
	seek_to=t;
}

void SetVolume(int v)
{
	mod.outMod->SetVolume(volume=v);
}

void SetPan(int p)
{
	mod.outMod->SetPan(pan=p);
}

In_Module mod=
{
	IN_VER,
	"",
	0,0,
	
	fmts_string,
	
	1,
	1,

	Config,
	About,

	Init,
	Quit,

	GetFileInfo,
	InfoBox,
	
	IsOurFile,
	Play,
	Pause,
	UnPause,
	IsPaused,
	Stop,

	GetLength,
	GetOutputTime,
	SetOutputTime,

	SetVolume,
	SetPan,
	
	0,0,0,0,0,0,0,0,0,0,0,
	EQSet,

	0,
	0,
};

extern "C"
{
	__declspec( dllexport ) In_Module * winampGetInModule2()
	{
		return &mod;
	}
}
