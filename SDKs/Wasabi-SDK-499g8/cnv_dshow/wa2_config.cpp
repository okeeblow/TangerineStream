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
#include "main.h"
#include <commctrl.h>
#include "resource.h"
#include "ds2.h"
#include <math.h>


#pragma warning(disable:4800)


FADE_CFG
cfg_fade_start={
	333,"start",0,1	
},cfg_fade_firststart={
	333,"first start",0,1
},cfg_fade_stop={
	333,"end of song",0,1	
},cfg_fade_pause={
	333,"pause/stop",1,1	
},cfg_fade_seek={
	333,"seek",1,1
};


#define N_FADES 5
static FADE_CFG * fades[N_FADES]={&cfg_fade_start,&cfg_fade_firststart,&cfg_fade_stop,&cfg_fade_pause,&cfg_fade_seek};

UINT cfg_def_fade=333;

bool cfg_oldpause=0;

extern Out_Module mod;
extern UINT cfg_buf_ms,cfg_prebuf2,cfg_sil_db,cfg_trackhack;
extern GUID cfg_dev2;
extern bool cfg_wait,cfg_killsil,cfg_volume;

#define yesno(x) ((x) ? "yes" : "no")

static char * rstrcpy(char * s1,const char * s2)
{
	while(*s2) *(s1++)=*(s2++);
	return s1;
}

static void format_fade(char * txt,FADE_CFG * c)
{
	txt=rstrcpy(txt,"on ");
	txt=rstrcpy(txt,c->name);
	if (!c->on) txt=rstrcpy(txt," (disabled)");
	else if (!c->usedef)
	{
		char tmp[16];
		wsprintf(tmp," (%u ms)",c->time);
		txt=rstrcpy(txt,tmp);
	}
	*txt=0;
}

static void add_fade(HWND w,FADE_CFG * cfg)
{
	FADE_CFG * c=(FADE_CFG*)ds2_malloc(sizeof(FADE_CFG));
	*c=*cfg;
	char txt[256];
	format_fade(txt,c);
	UINT i=SendMessage(w,LB_ADDSTRING,0,(long)txt);
	SendMessage(w,LB_SETITEMDATA,i,(long)c);
}

static void toggle_customfade(HWND wnd,int b)
{
	EnableWindow(GetDlgItem(wnd,IDC_TIME),b);
	EnableWindow(GetDlgItem(wnd,IDC_SPIN1),b);
	EnableWindow(GetDlgItem(wnd,IDC_STATIC_MS),b);
}

static BOOL WINAPI FadeProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(wnd,DWL_USER,lp);
			FADE_CFG * c=(FADE_CFG *)lp;
			char txt[256];
			wsprintf(txt,"Fade on %s - settings",c->name);
			SetWindowText(wnd,txt);
			SendDlgItemMessage(wnd,IDC_SPIN1,UDM_SETRANGE,0,MAKELONG(20000,0));
			SetDlgItemInt(wnd,IDC_TIME,c->time,0);
			SendDlgItemMessage(wnd,IDC_ENABLED,BM_SETCHECK,c->on,0);
			SendDlgItemMessage(wnd,IDC_FADETIME,BM_SETCHECK,!c->usedef,0);
			toggle_customfade(wnd,!c->usedef);
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_FADETIME:
			toggle_customfade(wnd,SendMessage((HWND)lp,BM_GETCHECK,0,0));
			break;
		case IDOK:
			{
				FADE_CFG * c=(FADE_CFG*)GetWindowLong(wnd,DWL_USER);
				c->time = GetDlgItemInt(wnd,IDC_TIME,0,0);
				c->on = (bool)SendDlgItemMessage(wnd,IDC_ENABLED,BM_GETCHECK,0,0);
				c->usedef = !SendDlgItemMessage(wnd,IDC_FADETIME,BM_GETCHECK,0,0);
			}
			EndDialog(wnd,1);
			break;
		case IDCANCEL:
			EndDialog(wnd,0);
			break;
		}
	}
	return 0;
}





static void update_prebuf_1(HWND wnd)
{
	char zz[128];
	wsprintf(zz,"%u ms",SendDlgItemMessage(wnd,IDC_PREBUFFER_1,TBM_GETPOS,0,0));
	SetDlgItemText(wnd,IDC_PREBUF_DISP_1,zz);
}

static void update_prebuf_2(HWND wnd)
{
	char zz[128];
	wsprintf(zz,"%u ms",SendDlgItemMessage(wnd,IDC_PREBUFFER_2,TBM_GETPOS,0,0));
	SetDlgItemText(wnd,IDC_PREBUF_DISP_2,zz);
}

#ifdef DS2_CRT_HACKS
double _stdcall f_pow(double a,double b);
#define pow f_pow
#endif

#define BUFFER_SCALE 4000.0

static UINT cur_buffer;


static UINT get_buffer(HWND wnd)
{
	if (cur_buffer) return cur_buffer;

	//0-BUFFER_SCALE => 200-20000	
	int z=SendDlgItemMessage(wnd,IDC_BUFFER,TBM_GETPOS,0,0);
	return cur_buffer=(UINT) ( 0.5 + 200.0*pow(100.0,(double)z/BUFFER_SCALE) );

}

static void set_buffer(HWND wnd,UINT b)
{
	cur_buffer=b;
	SendDlgItemMessage(wnd,IDC_BUFFER,TBM_SETPOS,1,(long) ( 0.5 + BUFFER_SCALE * log( (double)b/200.0 ) / log( 100.0 ) ));
}

static void update_prebuf_range(HWND wnd)
{
	UINT max=get_buffer(wnd);
	if (max>0x7FFF) max=0x7FFF;
	SendDlgItemMessage(wnd,IDC_PREBUFFER_1,TBM_SETRANGE,1,MAKELONG(0,max));
	SendDlgItemMessage(wnd,IDC_PREBUFFER_2,TBM_SETRANGE,1,MAKELONG(0,max));
}


static void update_buf(HWND wnd)
{
	char zz[128];
	wsprintf(zz,"%u ms",get_buffer(wnd));
	SetDlgItemText(wnd,IDC_BUF_DISP,zz);
}

BOOL WINAPI CfgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(wnd,IDC_SPIN3,UDM_SETRANGE,0,MAKELONG(0x7FFF,0));
		SetDlgItemInt(wnd,IDC_FADE,cfg_def_fade,0);
		SendDlgItemMessage(wnd,IDC_PAUSEFADE2,BM_SETCHECK,cfg_oldpause,0);
		{
			HWND w=GetDlgItem(wnd,IDC_DEVICE);
			DS2::DS_DEV * d=DS2::ds_devs;
			bool dev_set=0;
			UINT n=0;
			while(d)
			{
				SendMessage(w,CB_ADDSTRING,0,(long)d->name);
				if (!dev_set && d->guid && *d->guid==cfg_dev2)
				{
					SendMessage(w,CB_SETCURSEL,n,0);
					dev_set=1;
				}
				n++;
				d=d->next;
			}
			if (!dev_set) SendMessage(w,CB_SETCURSEL,0,0);

			w=GetDlgItem(wnd,IDC_DB);
			SendMessage(w,TBM_SETRANGE,0,MAKELONG(150,2000));
			SendMessage(w,TBM_SETPOS,1,cfg_sil_db);

			w=GetDlgItem(wnd,IDC_LIST);
			for(n=0;n<N_FADES;n++)
			{
				add_fade(w,fades[n]);
			}

		}
		SendDlgItemMessage(wnd,IDC_WAITx,BM_SETCHECK,cfg_wait,0);
		SendDlgItemMessage(wnd,IDC_VOLUME,BM_SETCHECK,cfg_volume,0);

		SendDlgItemMessage(wnd,IDC_KILLSIL,BM_SETCHECK,cfg_killsil,0);
		SendDlgItemMessage(wnd,IDC_CREATE_PRIMARY,BM_SETCHECK,cfg_createprimary,0);

		SendDlgItemMessage(wnd,IDC_BUFFER,TBM_SETRANGE,0,MAKELONG(0,(int)BUFFER_SCALE));
		set_buffer(wnd,cfg_buf_ms);
		update_prebuf_range(wnd);
		SendDlgItemMessage(wnd,IDC_PREBUFFER_1,TBM_SETPOS,1,cfg_prebuf2);
		SendDlgItemMessage(wnd,IDC_PREBUFFER_2,TBM_SETPOS,1,cfg_trackhack);
		update_prebuf_1(wnd);
		update_prebuf_2(wnd);
		update_buf(wnd);

		CfgProc(wnd,WM_HSCROLL,0,(long)GetDlgItem(wnd,IDC_DB));
		return 1;
	case WM_HSCROLL:
		switch(GetWindowLong((HWND)lp,GWL_ID))
		{
		case IDC_DB:
			{
				UINT foo=SendDlgItemMessage(wnd,IDC_DB,TBM_GETPOS,0,0);
				char zz[16];
				wsprintf(zz,"-%u.%u dB",foo/10,foo%10);
				SetDlgItemText(wnd,IDC_DB_DISPLAY,zz);
			}
			break;
		case IDC_BUFFER:
			cur_buffer=0;
			update_buf(wnd);
			update_prebuf_range(wnd);
			update_prebuf_1(wnd);
			update_prebuf_2(wnd);
			break;
		case IDC_PREBUFFER_1:
			update_prebuf_1(wnd);
			break;
		case IDC_PREBUFFER_2:
			update_prebuf_2(wnd);
			break;
		}
		break;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_INFO:
			{
				DSCAPS caps;
				UINT dev=SendDlgItemMessage(wnd,IDC_DEVICE,CB_GETCURSEL,0,0);
				if (dev==-1 || !DS2::getcaps(dev,&caps))
				{
					MessageBox(wnd,"Can't get device info.",0,MB_ICONERROR);
				}
				else
				{
					char blah[1024];
					wsprintf(blah,
						"Certified: %s\n"
						"Supports continuous sample rates: %s\n"
						"Emulated: %s\n"
						"Supports sample rates from %u Hz to %u Hz\n"
						"Total hardware memory: %u bytes\n"
						"Max hardware-mixed streaming buffers: %u"
						,
						yesno(caps.dwFlags&DSCAPS_CERTIFIED),
						yesno(caps.dwFlags&DSCAPS_CONTINUOUSRATE),
						yesno(caps.dwFlags&DSCAPS_EMULDRIVER),
						caps.dwMinSecondarySampleRate,caps.dwMaxSecondarySampleRate,
						caps.dwTotalHwMemBytes,
						caps.dwMaxHwMixingStreamingBuffers
						);
					MessageBox(wnd,blah,"Device info",MB_ICONINFORMATION);
				}
			}

			break;
		case IDC_RTFM:
			ShellExecute(0,"open","http://www.blorp.com/~peter/ds_manual.htm",0,0,SW_SHOW);
			break;
		case (LBN_DBLCLK<<16)|IDC_LIST:
			{
				UINT idx=SendMessage((HWND)lp,LB_GETCURSEL,0,0);
				if (idx!=-1)
				{
					FADE_CFG * c=(FADE_CFG*)SendMessage((HWND)lp,LB_GETITEMDATA,idx,0);
					if (DialogBoxParam(mod.hDllInstance,(char*)IDD_FADE,wnd,FadeProc,(long)c))
					{
						char txt[256];
						format_fade(txt,c);
						SendMessage((HWND)lp,LB_DELETESTRING,idx,0);
						SendMessage((HWND)lp,LB_INSERTSTRING,idx,(long)txt);
						SendMessage((HWND)lp,LB_SETITEMDATA,idx,(long)c);
					}
				}
			}
			break;
		case IDOK:

			cfg_buf_ms=get_buffer(wnd);
			cfg_prebuf2=SendDlgItemMessage(wnd,IDC_PREBUFFER_1,TBM_GETPOS,0,0);
			cfg_trackhack=SendDlgItemMessage(wnd,IDC_PREBUFFER_2,TBM_GETPOS,0,0);

			{
				UINT n=GetDlgItemInt(wnd,IDC_FADE,0,0);
				/*if (n<0) n=0;
				else */if (n>50000) n=50000;
				cfg_def_fade=n;

				HWND w=GetDlgItem(wnd,IDC_LIST);
				for(n=0;n<N_FADES;n++)
				{
					FADE_CFG* c=(FADE_CFG*)SendMessage(w,LB_GETITEMDATA,n,0);
					*fades[n]=*c;
					ds2_free(c);
				}

			}
			cfg_oldpause=(bool)SendDlgItemMessage(wnd,IDC_PAUSEFADE2,BM_GETCHECK,0,0);
			
			{
				int n=SendDlgItemMessage(wnd,IDC_DEVICE,CB_GETCURSEL,0,0);
				DS2::DS_DEV * d=DS2::ds_devs;
				while(n)
				{
					d=d->next;
					n--;
				}
				if (d && d->guid) cfg_dev2=*d->guid;
				else memset(&cfg_dev2,0,sizeof(cfg_dev2));
			}
			
			cfg_volume=(bool)SendDlgItemMessage(wnd,IDC_VOLUME,BM_GETCHECK,0,0);
			cfg_wait=(bool)SendDlgItemMessage(wnd,IDC_WAITx,BM_GETCHECK,0,0);
			cfg_killsil=SendDlgItemMessage(wnd,IDC_KILLSIL,BM_GETCHECK,0,0);
			cfg_sil_db=SendDlgItemMessage(wnd,IDC_DB,TBM_GETPOS,0,0);
			cfg_createprimary=(bool)SendDlgItemMessage(wnd,IDC_CREATE_PRIMARY,BM_GETCHECK,0,0);
/*			if (1)//cfg_preset<N_PRESETS)
			{
				cfg_buf_ms=presets[cfg_preset].buffer;
				cfg_prebuf2=presets[cfg_preset].prebuf;

				UINT maxfade=cfg_fade_firststart.get_time();
				UINT f1=cfg_fade_start.get_time();
				if (f1>maxfade) maxfade=f1;
				f1=cfg_fade_seek.get_time();
				if (f1>maxfade) maxfade=f1;
				f1=cfg_fade_pause.get_time();
				if (f1>maxfade) maxfade=f1;
				f1=cfg_fade_stop.get_time();
				if (f1>maxfade) maxfade=f1;
				if (maxfade>cfg_buf_ms)
				{
					cfg_prebuf2=MulDiv(cfg_prebuf2,cfg_buf_ms,maxfade);
					cfg_buf_ms=maxfade;
				}
			}*/
			EndDialog(wnd,1);
			break;
		case IDCANCEL:
			{
				HWND w=GetDlgItem(wnd,IDC_LIST);
				UINT n;
				for(n=0;n<N_FADES;n++)
				{
					ds2_free((void*)SendMessage(w,LB_GETITEMDATA,n,0));
				}
				
			}
			EndDialog(wnd,0);
			break;
		case IDC_BUF_RESET:
			set_buffer(wnd,2000);
			update_prebuf_range(wnd);
			SendDlgItemMessage(wnd,IDC_PREBUFFER_1,TBM_SETPOS,1,500);
			SendDlgItemMessage(wnd,IDC_PREBUFFER_2,TBM_SETPOS,1,200);
			update_prebuf_1(wnd);
			update_prebuf_2(wnd);
			update_buf(wnd);
			SendDlgItemMessage(wnd,IDC_CREATE_PRIMARY,BM_SETCHECK,(GetVersion()&0x80000000) ? 1 : 0,0);
			break;
		}
		break;
	}
	return 0;
}

void Config(HWND w)
{
	DS2::init(mod.hDllInstance);
	if (DialogBox(mod.hDllInstance,(char*)IDD_CONFIG,w,CfgProc))
	{
		UINT maxfade=cfg_fade_firststart.get_time();
		UINT f1=cfg_fade_start.get_time();
		if (f1>maxfade) maxfade=f1;
		f1=cfg_fade_seek.get_time();
		if (f1>maxfade) maxfade=f1;
		f1=cfg_fade_pause.get_time();
		if (f1>maxfade) maxfade=f1;
		if (maxfade>cfg_buf_ms)
		{
//			cfg_buf_ms=maxfade;
			char foo[256];
			wsprintf(foo,"Some fade times are bigger than buffer length; in order to get what you want, please increase buffer size to %u ms.\nWould you like your settings to be automatically corrected ?",maxfade);
			if (MessageBox(w,foo,"Warning",MB_ICONEXCLAMATION|MB_YESNO)==IDYES) cfg_buf_ms=maxfade;
		}
	}
}

static char _dllfile[MAX_PATH];
static char * dllfile;
static char inifile[MAX_PATH];

static UINT atoui(char* s)
{
	int ret=0;
	while(*s>='0' && *s<='9') {ret=10*ret+(*s-'0');s++;}
	return ret;
}

static int _do_var(int v,char* n,bool s)
{
	if (s)
	{
		char tmp[32];
		wsprintf(tmp,"%u",v);
		WritePrivateProfileString(dllfile,n,tmp,inifile);
		return v;
	}
	else
	{
		char tmp[64],tmp_s[32];
		wsprintf(tmp_s,"%u",v);
		GetPrivateProfileString(dllfile,n,tmp_s,tmp,64,inifile);
		return atoui(tmp);
	}

}

#define do_var(V) {V=_do_var(V,#V,s);}
/*
static void _do_string(char* file,char* string,char* n,bool s)
{
	if (s)
	{
		WritePrivateProfileString(dllfile,n,string,inifile);
	}
	else
	{
		char tmp[MAX_PATH];
		strcpy(tmp,string);
		GetPrivateProfileString(dllfile,n,tmp,string,MAX_PATH,inifile);
	}
}

#define do_string(V) _do_string(V,#V,s)
*/

static void _do_struct(void * ptr,UINT siz,char * n,bool s)
{
	if (s)
	{
		WritePrivateProfileStruct(dllfile,n,ptr,siz,inifile);
	}
	else
	{
		GetPrivateProfileStruct(dllfile,n,ptr,siz,inifile);
	}
}

#define do_struct(V) _do_struct(&V,sizeof(V),#V,s)

#define do_fade(X) {do_var(X.time);do_var(X.on);do_var(X.usedef);}


#ifndef _DEBUG
//avoid strchr crt references
char * t_strchr(const char * s,int c)
{
	while(*s && *s!=c) s++;
	return *s ? (char*)s : 0;
}

char * t_strrchr(const char * s,int c)
{
	char * r=0;
	while(*s) {if (*s==c) r=(char*)s; s++;}
	return r;
}

#define strchr t_strchr
#define strrchr t_strrchr
#endif

void do_cfg(bool s)
{
	if (!inifile[0])
	{
		char *p;
		GetModuleFileName(0,inifile,sizeof(inifile));
		p=inifile+strlen(inifile);
		while (p >= inifile && *p != '.') p--;
		strcpy(++p,"ini");
	}
	if (!dllfile)
	{
		GetModuleFileName(mod.hDllInstance,_dllfile,sizeof(_dllfile));
		dllfile=strrchr(_dllfile,'\\');
		if (!dllfile) dllfile=_dllfile;
		else dllfile++;
		char * p=strchr(dllfile,'.');
		if (p) *p=0;
	}

	do_var(cfg_buf_ms);
	do_var(cfg_prebuf2);
	do_var(cfg_oldpause);
	do_var(cfg_def_fade);
	do_var(cfg_wait);
	do_struct(cfg_dev2);
	do_var(cfg_sil_db);
	do_var(cfg_killsil);
	do_var(cfg_createprimary);
	do_var(cfg_trackhack);
	do_var(cfg_volume);

	do_fade(cfg_fade_start);
	do_fade(cfg_fade_firststart);
	do_fade(cfg_fade_stop);
	do_fade(cfg_fade_pause);
	do_fade(cfg_fade_seek);
	
}

