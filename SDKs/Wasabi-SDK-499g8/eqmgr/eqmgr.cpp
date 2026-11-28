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
#include "eqmgr.h"
#include "resource.h"

#include <studio/services/servicei.h>
#include <common/popup.h>
#include <studio/api.h>
#include <common/selectfile.h>
#include <common/savefile.h>
#include <common/xlatstr.h>
#include <common/metadatasvc.h>
#include <core/corehandle.h>
#include <bfc/attribs/attribs.h>

static WACEqMgr wac;
WAComponentClient *the = &wac;

// {B66F7CE8-8BB9-4daf-95F5-4C45B05CF1B7}
static const GUID guid = 
{ 0xb66f7ce8, 0x8bb9, 0x4daf, { 0x95, 0xf5, 0x4c, 0x45, 0xb0, 0x5c, 0xf1, 0xb7 } };
// {14D21FF1-A1FC-4161-8733-146F8D16EB55}
static const GUID table_guid = 
{ 0x14d21ff1, 0xa1fc, 0x4161, { 0x87, 0x33, 0x14, 0x6f, 0x8d, 0x16, 0xeb, 0x55 } };

static String eqdir1, eqdir2, last_preset;

#define MT_PREAMP "Preamp"

WACEqMgr::WACEqMgr() {
  MetaDataSvc *mds = new MetaDataSvc("EQ", table_guid);
  mds->addColumn(MT_PREAMP, MDT_FLOAT, FALSE, FALSE);
  registerService(new MetaDataSvcFactory(mds));
}

GUID WACEqMgr::getGUID() {
  return guid;
}

void WACEqMgr::onCreate() {
  eqdir1=api->getAppPath();
  eqdir1+=DIRCHARSTR;
  eqdir1+="winamp.q1";
  eqdir2=api->getAppPath();
  eqdir2+=DIRCHARSTR;
  eqdir2+="winamp.q2";

  api->core_addCallback(0,this);
  
  const char *cmdline=WASABI_API_APP->main_getCommandLine();
  if(STRSTR(cmdline,"/INSTALL")) {
		struct {
		  char *s; 
			unsigned char tab[10];
		}	eqsets[] =  {
				  {"Default",{31,31,31,31,31,31,31,31,31,31}},
				  {"Classical",{31,31,31,31,31,31,44,44,44,48}},
				  {"Club",{31,31,26,22,22,22,26,31,31,31}},
				  {"Dance",{16,20,28,32,32,42,44,44,32,32}},
				  {"Laptop speakers/headphones",{24,14,23,38,36,29,24,16,11,8}},
				  {"Large hall",{15,15,22,22,31,40,40,40,31,31}},
				  {"Party",{20,20,31,31,31,31,31,31,20,20}},
          {"Pop",{35,24,20,19,23,34,36,36,35,35}},
			    {"Reggae",{31,31,33,42,31,21,21,31,31,31}},
          {"Rock",{19,24,41,45,38,25,17,14,14,14}},
          {"Soft",{24,29,34,36,34,25,18,16,14,12}},
          {"Ska",{36,40,39,33,25,22,17,16,14,16}},
          {"Full Bass",{16,16,16,22,29,39,46,49,50,50}},
          {"Soft Rock",{25,25,28,33,39,41,38,33,27,17}},
          {"Full Treble",{48,48,48,39,27,14,6,6,6,4}},
          {"Full Bass & Treble",{20,22,31,44,40,29,18,14,12,12}},
          {"Live",{40,31,25,23,22,22,25,27,27,28}},
          {"Techno",{19,22,31,41,40,31,19,16,16,17}},
		};

		for (int x = 0; x < sizeof(eqsets)/sizeof(eqsets[0]); x ++)
				writeEQfile_init(eqdir1,eqsets[x].s,eqsets[x].tab);
  }
}

void WACEqMgr::onDestroy() {
  api->core_delCallback(0,this);
}

int WACEqMgr::onCommand(const char *cmd, int param1, int param2, void *ptr, int ptrlen) {
  if(STRCASEEQL(cmd,"do_presets_menu")) doPresetsMenu();
  return 0;
}

int WACEqMgr::corecb_onSetNextFile(const char *playstring) {
  if(api->core_getEqAuto(0)) doAutoEq(playstring);
  return 0;
}

void WACEqMgr::doPresetsMenu() {
  PopupMenu *menu = new PopupMenu();
  PopupMenu *loadmenu = new PopupMenu();
  loadmenu->addCommand("Preset...",1);
  loadmenu->addCommand("Auto-load preset...",2);
  loadmenu->addCommand("Default",3);
  loadmenu->addSeparator();
  loadmenu->addCommand("From EQF...",4);
  menu->addSubMenu(loadmenu,"Load");
  PopupMenu *savemenu = new PopupMenu();
  savemenu->addCommand("Preset...",5);
  savemenu->addCommand("Auto-load preset...",6);
  savemenu->addCommand("Default",7);
  savemenu->addSeparator();
  savemenu->addCommand("To EQF...",8);
  menu->addSubMenu(savemenu,"Save");
  PopupMenu *delmenu = new PopupMenu();
  delmenu->addCommand("Preset...",9);
  delmenu->addCommand("Auto-load preset...",10);
  menu->addSubMenu(delmenu,"Delete");
  int ret=menu->popAtMouse();
  delete(menu);

  switch(ret) {
  case 1: loadPresetDialog(); break;
  case 2: loadAutoPresetDialog(); break;
  case 3: loadDefaultPreset(); break;
  case 4: loadFromEQF(); break;
  case 5: savePresetDialog(); break;
  case 6: saveAutoPresetDialog(); break;
  case 7: saveDefaultPreset(); break;
  case 8: saveToEQF(); break;
  case 9: deletePresetDialog(); break;
  case 10: deleteAutoPresetDialog(); break;
  }
}

void WACEqMgr::saveDefaultPreset() {
  writeEQfile(eqdir1,"Default");
}

void WACEqMgr::loadDefaultPreset() {
  readEQfile(eqdir1,"Default");
}

void WACEqMgr::loadFromEQF() {
  SelectFile sf(api->main_getRootWnd(), "Select file to read", "...");
  sf.runSelector("files", FALSE, "EQ Setting Files (*.EQF)\0*.EQF\0All files\0*.*\0");
  if(!sf.getNumFiles()) return;
  readEQfile(sf.enumFilename(0),"Entry1");  
}

void WACEqMgr::saveToEQF() {
  SaveFileWnd sf("EQF file");
  if(sf.getSaveFile(api->main_getRootWnd(),"EQ Setting Files (*.EQF)\0*.EQF\0All files\0*.*\0","eqf"))
    writeEQfile(sf.getFilename(),"Entry1");
}

static const char sig[] = "Winamp EQ library file v1.1\x1A!--";

int WACEqMgr::readEQfile(const char *file, const char *name, int apply_preamp) {
  unsigned char eq_tab[10]={31,31,31,31,31,31,31,31,31,31};
  unsigned char config_preamp=31;

  char s[2048];
  int a;
  FILE *fh=FOPEN(file,"rb");
  if (!fh) return 1;
  FREAD(s,1,strlen(sig),fh);
  if (MEMCMP(s,sig,strlen(sig))) {
    FCLOSE(fh);
    return 1;
  }
  while (1) {
    a=FREAD(s,1,_MAX_FNAME+1+sizeof(eq_tab)+sizeof(config_preamp),fh);
    if (a != _MAX_FNAME+1+sizeof(eq_tab)+sizeof(config_preamp)) {
      FCLOSE(fh);
      return 1;
    }
    if (!STRICMP(name,s)) break;
  }
  FCLOSE(fh);
  memcpy(eq_tab,s + _MAX_FNAME+1,sizeof(eq_tab));
  memcpy(&config_preamp, s + _MAX_FNAME+1+sizeof(eq_tab),sizeof(config_preamp));
  if (apply_preamp) api->core_setEqPreamp(0,-((int)config_preamp-31)*4);
  for(int i=0;i<10;i++) api->core_setEqBand(0,i,-((int)eq_tab[i]-31)*4);
    return 0;
}

int WACEqMgr::writeEQfile(const char *file, const char *name) {
  unsigned char eq_tab[10]={31,31,31,31,31,31,31,31,31,31};
  unsigned char config_preamp=(-api->core_getEqPreamp(0)/4)+31;
  for(int i=0;i<10;i++) eq_tab[i]=(-api->core_getEqBand(0,i)/4)+31;

  //CT> justin's code, uses SetEndOfFile...
	HANDLE hFile;
	char s[2048];
	unsigned long a;
	hFile = CreateFile(file,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE) return 1;
	ReadFile(hFile,s,STRLEN(sig),&a,NULL);
	if (MEMCMP(s,sig,STRLEN(sig))) {
		SetFilePointer(hFile,0,NULL,FILE_BEGIN);
		WriteFile(hFile,sig,STRLEN(sig),&a,NULL);
		SetEndOfFile(hFile);
	}
	while (1) {
		ReadFile(hFile,s,_MAX_FNAME+1+sizeof(eq_tab)+sizeof(config_preamp),&a,NULL);
		if (a != _MAX_FNAME+1+sizeof(eq_tab)+sizeof(config_preamp))	{
			SetFilePointer(hFile,0,NULL,FILE_END);
			break;
		}
		if (!STRICMP(name,s)) {
			SetFilePointer(hFile,-((long)a),NULL,FILE_CURRENT);
			break;
		}
	}
	STRCPY(s,name);
	MEMCPY(s + _MAX_FNAME+1,eq_tab,sizeof(eq_tab));
	MEMCPY(s + _MAX_FNAME+1+sizeof(eq_tab),&config_preamp,sizeof(config_preamp));
	WriteFile(hFile,s,_MAX_FNAME+1+sizeof(eq_tab)+sizeof(config_preamp),&a,NULL);
	CloseHandle(hFile);
	return 0;
}

int WACEqMgr::writeEQfile_init(const char *file, char *name, unsigned char *tab) {
	unsigned char preamp=31;
	HANDLE hFile;
	char s[2048];
	unsigned long a;
	hFile = CreateFile(file,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE) return 1;
	ReadFile(hFile,s,strlen(sig),&a,NULL);
	if (memcmp(s,sig,strlen(sig)))
	{
		SetFilePointer(hFile,0,NULL,FILE_BEGIN);
		WriteFile(hFile,sig,strlen(sig),&a,NULL);
		SetEndOfFile(hFile);
	}
	while (1)
	{
		ReadFile(hFile,s,_MAX_FNAME+1+10+1,&a,NULL);
		if (a != _MAX_FNAME+1+10+1)
		{
			SetFilePointer(hFile,0,NULL,FILE_END);
			break;
		}
		if (!stricmp(name,s))
		{
			CloseHandle(hFile);
			return 0; // don't write it
		}
	}
	strcpy(s,name);
	memcpy(s + _MAX_FNAME+1,tab,10);
	memcpy(s + _MAX_FNAME+1+10,&preamp,1);
	WriteFile(hFile,s,_MAX_FNAME+1+10+1,&a,NULL);
	CloseHandle(hFile);
	return 0;
}

void WACEqMgr::deleteEQfile(const char *file, const char *name) {
	HANDLE hFile, hFileOut;
	char s[2048];
	unsigned long a;
	hFile = CreateFile(file,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;
	wsprintf(s,"%s.TMP",file);
	hFileOut = CreateFile(s,GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFileOut == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return;
	}
	ReadFile(hFile,s,STRLEN(sig),&a,NULL);
	WriteFile(hFileOut,s,STRLEN(sig),&a,NULL);
	while (1)
	{
		ReadFile(hFile,s,_MAX_FNAME+1+/*sizeof(eq_tab)*/10+/*sizeof(config_preamp)*/1,&a,NULL);
		if (a != _MAX_FNAME+1+/*sizeof(eq_tab)*/10+/*sizeof(config_preamp)*/1) break;
		if (STRICMP(name,s)) WriteFile(hFileOut,s,a,&a,NULL);
	}
	CloseHandle(hFile);
	CloseHandle(hFileOut);
	DeleteFile(file);
	SPRINTF(s,"%s.TMP",file);
	MoveFile(s,file);
	return;
}

static void addEQtolistbox(const char *file, HWND listbox)
{
	HANDLE hFile;
	char s[2048];
	unsigned long l;
	hFile = CreateFile(file,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;
	ReadFile(hFile,s,strlen(sig),&l,NULL);
	if (MEMCMP(s,sig,strlen(sig))) {
		CloseHandle(hFile);
		return;
	}
	while (1)	{
		unsigned long a;
		ReadFile(hFile,s,_MAX_FNAME+1+/*sizeof(eq_tab)*/10+/*sizeof(config_preamp)*/1,&a,NULL);
		if (a != _MAX_FNAME+1+/*sizeof(eq_tab)*/10+/*sizeof(config_preamp)*/1) break;
		SendMessage(listbox,LB_ADDSTRING,0,(LPARAM)s);
	}
	CloseHandle(hFile);
}

//CT> yes, this is win32 stuff cause i'm waiting for Francis to give me group stuff for dialogs
static BOOL CALLBACK loadpresetProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	int x;
	static int eqtbk[10];
	static int eqpa;
	static char lpp[512];
	switch (uMsg) 
	{
		case WM_INITDIALOG:
			addEQtolistbox(eqdir1,GetDlgItem(hwndDlg,IDC_LOADEQLIST));
			x = SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)last_preset.getValue());
			if (x != LB_ERR)
				SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,x,0);
			else
				SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,-1,0);
			if(!last_preset.isempty()) strcpy(lpp,last_preset);
      else lpp[0]=0;
      {
  			eqpa=api->core_getEqPreamp(0);
        for(int i=0;i<10;i++) eqtbk[i]=api->core_getEqBand(0,i);
      }
		return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
          {
            char tmp[2048];
						SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETTEXT,		
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCURSEL,0,0),
							  (LPARAM) tmp);
            last_preset=tmp;
          }
            wac.readEQfile(eqdir1,last_preset);
					EndDialog(hwndDlg,0);
				return FALSE;
				case IDCANCEL:
					api->core_setEqPreamp(0,eqpa);
          {
            for(int i=0;i<10;i++) api->core_setEqBand(0,i,eqtbk[i]);
          }
					last_preset=lpp;
					EndDialog(hwndDlg,1);
				return FALSE;
				case IDC_LOADEQLIST:
					if (HIWORD(wParam) == LBN_DBLCLK)
						SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
            {
              char tmp[2048];
  						SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETTEXT,		
	  						SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCURSEL,0,0),
		  					  (LPARAM) tmp);
              last_preset=tmp;
            }
            wac.readEQfile(eqdir1,last_preset);
					}
				return FALSE;			
			}
		return FALSE;
	}
return 0;
}

static BOOL CALLBACK loadmp3Proc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	static int eqtbk[10];
	static int eqpa;
	char buf[1024];
	switch (uMsg) 
	{
		case WM_INITDIALOG:
			addEQtolistbox(eqdir2,GetDlgItem(hwndDlg,IDC_LOADEQLIST));
			SetWindowText(hwndDlg,_("Load auto-load preset"));
			{
				int x;
        const char *FileName=api->core_getCurrent(0);
        if(FileName) STRCPY(buf,Std::scanstr_back(FileName,"\\",FileName-1)+1);
        else buf[0]=0;
				SetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),buf);
				x = SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)buf);
				if (x != LB_ERR)
					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,x,0);
				else
					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,-1,0);
			}
      {
  			eqpa=api->core_getEqPreamp(0);
        for(int i=0;i<10;i++) eqtbk[i]=api->core_getEqBand(0,i);
      }
		return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
						SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETTEXT,		
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCURSEL,0,0),
							  (LPARAM) buf);
            wac.readEQfile(eqdir2,buf);
					EndDialog(hwndDlg,0);
				return FALSE;
				case IDCANCEL:
					api->core_setEqPreamp(0,eqpa);
          {
            for(int i=0;i<10;i++) api->core_setEqBand(0,i,eqtbk[i]);
          }
					EndDialog(hwndDlg,1);
				return FALSE;
				case IDC_LOADEQLIST:
					if (HIWORD(wParam) == LBN_DBLCLK)
						SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
						SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETTEXT,		
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCURSEL,0,0),
							  (LPARAM) buf);
            wac.readEQfile(eqdir2,buf);
						EnableWindow(GetDlgItem(hwndDlg,IDOK),1);
					}
				return FALSE;			
			}
		return FALSE;
	}
return 0;
}


static BOOL CALLBACK savepresetProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	char buf[1024];
	switch (uMsg) 
	{
		case WM_INITDIALOG:
			addEQtolistbox(eqdir1,GetDlgItem(hwndDlg,IDC_LOADEQLIST));
			{
				int x;
				if(!last_preset.isempty()) strcpy(buf,last_preset);
        else buf[0]=0;
				SetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),buf);
				x = SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)buf);
				if (x != LB_ERR)
					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,x,0);
				else
					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,-1,0);
			}

			SendMessage(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),EM_LIMITTEXT,_MAX_FNAME,0);

		return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
          {
            char tmp[2048];
					  GetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),tmp,sizeof(tmp)-1);
            last_preset=tmp;
          }
          wac.writeEQfile(eqdir1,last_preset);
					EndDialog(hwndDlg,0);
				return FALSE;
				case IDCANCEL:
					EndDialog(hwndDlg,1);
				return FALSE;
				case IDC_LOADEQLIST:
					if (HIWORD(wParam) == LBN_DBLCLK)
						SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
						SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETTEXT,		
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCURSEL,0,0),
								(LPARAM) buf);
						SetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),buf);
					}
				return FALSE;			
				case IDC_SAVEPRESET_EDIT:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						int x;
						GetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),buf,sizeof(buf)-1);
						if (strlen(buf)) EnableWindow(GetDlgItem(hwndDlg,IDOK),1);
						else EnableWindow(GetDlgItem(hwndDlg,IDOK),0);

						x = SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)buf);
						if (x != LB_ERR)
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,x,0);
						else
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,-1,0);
					}
			}
		return FALSE;
	}
  return 0;
}

static BOOL CALLBACK savemp3Proc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	char buf[1024];
	switch (uMsg) 
	{
		case WM_INITDIALOG:
			addEQtolistbox(eqdir2,GetDlgItem(hwndDlg,IDC_LOADEQLIST));
			{
				int x;
        const char *FileName=api->core_getCurrent(0);
        if(FileName) STRCPY(buf,Std::scanstr_back(FileName,"\\",FileName-1)+1);
        else buf[0]=0;
				SetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),buf);
				x = SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)buf);
				if (x != LB_ERR)
					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,x,0);
				else
					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,-1,0);
				SetWindowText(hwndDlg,_("Save auto-load preset"));
			}
			SendMessage(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),EM_LIMITTEXT,_MAX_FNAME,0);

		return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					GetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),buf,sizeof(buf)-1);
          wac.writeEQfile(eqdir2,buf);
					EndDialog(hwndDlg,0);
				return FALSE;
				case IDCANCEL:
					EndDialog(hwndDlg,1);
				return FALSE;
				case IDC_LOADEQLIST:
					if (HIWORD(wParam) == LBN_DBLCLK)
						SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
						SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETTEXT,		
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCURSEL,0,0),
								(LPARAM) buf);
						SetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),buf);
					}
				return FALSE;			
				case IDC_SAVEPRESET_EDIT:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						int x;
						GetWindowText(GetDlgItem(hwndDlg,IDC_SAVEPRESET_EDIT),buf,sizeof(buf)-1);
						x = SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)buf);
						if (x != LB_ERR)
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,x,0);
						else
							SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,-1,0);
					}
			}
		return FALSE;
	}
  return 0;
}

static BOOL CALLBACK delpresetProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	int x;
	switch (uMsg) 
	{
		case WM_INITDIALOG:
			addEQtolistbox(eqdir1,GetDlgItem(hwndDlg,IDC_LOADEQLIST));
			if (SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCOUNT,0,0) == 0)
				EnableWindow(GetDlgItem(hwndDlg,IDOK),0);
			SetWindowText(hwndDlg,_("Delete preset"));
			SetWindowText(GetDlgItem(hwndDlg,IDOK),_("Delete..."));
			x = SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)last_preset.getValue());
			if (x != LB_ERR)
				SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,x,0);
			else
				SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,-1,0);

		return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
          {
            char tmp[2048];
  					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETTEXT,		
	  					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCURSEL,0,0),
						    (LPARAM) tmp);
            last_preset=tmp;
          }
          wac.deleteEQfile(eqdir1,last_preset);
					EndDialog(hwndDlg,0);
				return FALSE;
				case IDCANCEL:
					EndDialog(hwndDlg,1);
				return FALSE;
				case IDC_LOADEQLIST:
					if (HIWORD(wParam) == LBN_DBLCLK)
						SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
				return FALSE;			
			}
		return FALSE;
	}
  return 0;
}

static BOOL CALLBACK delmp3Proc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	int x;
	char buf[1024];
	switch (uMsg) 
	{
		case WM_INITDIALOG:
			addEQtolistbox(eqdir2,GetDlgItem(hwndDlg,IDC_LOADEQLIST));
			if (SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCOUNT,0,0) == 0)
				EnableWindow(GetDlgItem(hwndDlg,IDOK),0);
			SetWindowText(hwndDlg,_("Delete auto-load preset"));
			SetWindowText(GetDlgItem(hwndDlg,IDOK),_("Delete..."));
      {
        const char *FileName=api->core_getCurrent(0);
        if(FileName) STRCPY(buf,Std::scanstr_back(FileName,"\\",FileName-1)+1);
        else buf[0]=0;
      }
			x = SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_FINDSTRINGEXACT,(WPARAM)0,(LPARAM)buf);
			if (x != LB_ERR)
				SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,x,0);
			else
				SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_SETCURSEL,-1,0);

		return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETTEXT,		
						SendMessage(GetDlgItem(hwndDlg,IDC_LOADEQLIST),LB_GETCURSEL,0,0),
						  (LPARAM) buf);
          wac.deleteEQfile(eqdir2,buf);
					EndDialog(hwndDlg,0);
				return FALSE;
				case IDCANCEL:
					EndDialog(hwndDlg,1);
				return FALSE;
				case IDC_LOADEQLIST:
					if (HIWORD(wParam) == LBN_DBLCLK)
						SendMessage(hwndDlg,WM_COMMAND,IDOK,0);
				return FALSE;			
			}
		return FALSE;
	}
  return 0;
}

void WACEqMgr::loadPresetDialog() {
  DialogBox(gethInstance(),MAKEINTRESOURCE(IDD_LOADPRESET),GetActiveWindow(),loadpresetProc);
}

void WACEqMgr::savePresetDialog() {
  DialogBox(gethInstance(),MAKEINTRESOURCE(IDD_SAVEPRESET),GetActiveWindow(),savepresetProc);
}

void WACEqMgr::loadAutoPresetDialog() {
  DialogBox(gethInstance(),MAKEINTRESOURCE(IDD_LOADPRESET),GetActiveWindow(),loadmp3Proc);
}

void WACEqMgr::saveAutoPresetDialog() {
  DialogBox(gethInstance(),MAKEINTRESOURCE(IDD_SAVEPRESET),GetActiveWindow(),savemp3Proc);
}

void WACEqMgr::deletePresetDialog() {
  DialogBox(gethInstance(),MAKEINTRESOURCE(IDD_LOADPRESET),GetActiveWindow(),delpresetProc);
}

void WACEqMgr::deleteAutoPresetDialog() {
  DialogBox(gethInstance(),MAKEINTRESOURCE(IDD_LOADPRESET),GetActiveWindow(),delmp3Proc);
}

void WACEqMgr::doAutoEq(const char *playstring) {
  if (playstring == NULL) return;

  int autoset=0; // if this isn't set by the end of the function, we set the eq to 0 so we're not stuck with the last setting

  // try to lookup preamp setting from table
  float preamp = 0;
  int r = api->metadb_getMetaData(playstring, MT_PREAMP, (char*)&preamp, sizeof(float), MDT_FLOAT);
  if (r > 0) {
#define PREAMPMAX (12.f)
    preamp = MINMAX(preamp, -PREAMPMAX, PREAMPMAX);
    int ival = (int)((preamp/PREAMPMAX) * 127.0);
    CoreHandle().setEqPreamp(ival);
    autoset = 1;
  }

  const char *f=Std::scanstr_back(playstring,"\\",playstring-1)+1;
  if (!readEQfile(eqdir2,f)) autoset = 1;
  else if (!readEQfile(eqdir1,"Default", r <= 0)) autoset = 1;

  if (!autoset) api->core_setEqPreamp(0, 0);
}
