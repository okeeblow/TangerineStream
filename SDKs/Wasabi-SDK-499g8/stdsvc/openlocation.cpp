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
//NONPORTABLE

#include "precomp.h"
#include <windows.h>

#include "openlocation.h"
#include "resource.h"

#include "../studio/api.h"
#include "../bfc/rootwnd.h"

static String loc;

static void recent_add(const char *loc, char *which)
{
  int recent_num=WASABI_API_CONFIG->getIntPrivate("NumRecentURLs",16);
  char ls[MAX_PATH];
  StringPrintf s("RecentURL%s1",which);
  WASABI_API_CONFIG->getStringPrivate(s,ls,sizeof(ls),"");
  WASABI_API_CONFIG->setStringPrivate(s,loc);

  if (STRCMP(ls,loc)) for (int x = 1; x < recent_num; x ++)
  {
    char temp[MAX_PATH];
    StringPrintf s("RecentURL%s%d",which,x+1);
    WASABI_API_CONFIG->getStringPrivate(s,temp,sizeof(temp),"");
    WASABI_API_CONFIG->setStringPrivate(s,ls);
    if (!STRCMP(temp,loc)) break;
    STRCPY(ls,temp);
  }
}

static BOOL CALLBACK locProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  switch (uMsg) {
    case WM_INITDIALOG: {
      SendDlgItemMessage(hwndDlg,IDC_URL_NEW,CB_LIMITTEXT,WA_MAX_PATH-1,0);
      int recent_num=WASABI_API_CONFIG->getIntPrivate("NumRecentURLs",16);
      HWND h=GetDlgItem(hwndDlg,IDC_URL_NEW);
      for (int x = 0; x < recent_num; x ++) {
        char name[MAX_PATH];
        WASABI_API_CONFIG->getStringPrivate(StringPrintf("RecentURL%d",x+1),name,sizeof(name),"");
        if (!name[0]) break;
        SendMessage(h,CB_ADDSTRING,0,(LPARAM)name);
      }
      SetDlgItemText(hwndDlg,IDC_URL_NEW,loc);
    }          
    return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK: {
          char tmp[WA_MAX_PATH];
          GetDlgItemText(hwndDlg,IDC_URL_NEW,tmp,sizeof(tmp)-1);
          char *p=tmp;
          while (*p == ' ' || *p == '\t') p ++;
          if (!*p) EndDialog(hwndDlg,1);
          else {
            loc=tmp;
            recent_add(loc,"");
            EndDialog(hwndDlg,0);
          }
        }
        return FALSE;
        case IDCANCEL:
          EndDialog(hwndDlg,1);
        return FALSE;
      }
      return FALSE;
      case WM_CLOSE:
        EndDialog(hwndDlg,2);
      return FALSE;
  }
  return 0;
}

int OpenLocation::runSelector(RootWnd *parWnd, int type, int allow_multiple, const char *ident, const char *default_dir)
{
  confirmed=0;
  extern OSMODULEHANDLE g_modulehandle;
  int ret = DialogBox(g_modulehandle, MAKEINTRESOURCE(IDD_OPENLOC), parWnd->gethWnd(), locProc);
  ASSERT(ret != -1);	// resource not found maybe
  if (ret == 0) {
    char *l=(char *)loc.getValue();
    while (*l == ' ') l++;
    if (*l == '\\') {	// \ means take the following string literally
      l = l + 1;
    } else {
      if (!STRSTR(l,":/") && !STRSTR(l,":\\")) {
        char buf[WA_MAX_PATH+32];
        SPRINTF(buf,"http://%s",l);
        l=buf;
      }
    }
    if(*l) {
      location=l;
      confirmed=1;
      return 1;
    }
  }
  return 0;
}
