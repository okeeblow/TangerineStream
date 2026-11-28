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
#include "opendir.h"

#include "../common/choosedir.h"
#include "../common/xlatstr.h"
#include <shlobj.h>
#include "../common/readdir.h"

#ifdef WIN32
static HRESULT ResolveShortCut(HWND hwnd, LPCSTR pszShortcutFile, LPSTR pszPath)
{
  HRESULT hres;
  IShellLink* psl;
  char szGotPath[MAX_PATH];
  WIN32_FIND_DATA wfd;

  *pszPath = 0;   // assume failure

  hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (void **) &psl);
  if (SUCCEEDED(hres))
  {
    IPersistFile* ppf;

    hres = psl->QueryInterface(IID_IPersistFile, (void **) &ppf); // OLE 2!  Yay! --YO
    if (SUCCEEDED(hres))
    {
      WORD wsz[MAX_PATH];
      MultiByteToWideChar(CP_ACP, 0, pszShortcutFile, -1, wsz, MAX_PATH);

     hres = ppf->Load(wsz, STGM_READ);
     if (SUCCEEDED(hres))
     {
       hres = psl->Resolve(hwnd, SLR_ANY_MATCH);
       if (SUCCEEDED(hres))
       {
          strcpy(szGotPath, pszShortcutFile);
          hres = psl->GetPath(szGotPath, MAX_PATH, (WIN32_FIND_DATA *)&wfd,
                 SLGP_SHORTPATH );
		  strcpy(pszPath,szGotPath);
        }
      }
      ppf->Release();
    }
    psl->Release();
  }
  return SUCCEEDED(hres);
}
#endif

OpenDirectory::OpenDirectory() {
  files.setAutoSort(false);
}

OpenDirectory::~OpenDirectory() {
  cleanFiles();
}

void OpenDirectory::addDir(const char *zpath) {
  if (zpath[0] == '\0') return;	// empty path

  ReadDir dir(zpath);

  while (dir.next()) {
    const char *fn = dir.getFilename();
    ASSERT(fn != NULL);
    
    if (dir.isDir()) {
      if (recurseSubDirs) {
        String newpath(zpath);
        if (newpath.lastChar() != DIRCHAR) newpath.cat(DIRCHARSTR);
        newpath.cat(fn);
        addDir(newpath);
      }
    } else {	// normal file
#ifdef WIN32
      const char *ext = Std::extension(fn);
      if (ext != NULL && STRCASEEQL(ext, "lnk")) {	// shortcut
        // don't feel like it, somebody else port it who cares :p
      } else {
#endif
        // normal file
        for (int i = 0; i < exts.getNumItems(); i++) {
          if (STRCASEEQL(exts.q(i), ext)) {
            addFile(StringPrintf("%s%s%s", zpath, DIRCHARSTR, fn));
            break;
          }
        }
#ifdef WIN32
      }
#endif
    }	// not a dir
  } // while readdir
#if 0
  char path[WA_MAX_PATH];
  STRCPY(path, zpath);

	WIN32_FIND_DATA d;
	char filespec[MAX_PATH+15];
	HANDLE i;

  if (!path[0]) return;
 
	if (path[strlen(path)-1] == '\\') path[strlen(path)-1] = 0;
	

	wsprintf(filespec,"%s\\*.*",path);
	i =  FindFirstFile(filespec,&d);
	if (i != INVALID_HANDLE_VALUE)
	{
		do {
			if (recurse && d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && d.cFileName[0]!='.')
			{
				wsprintf(filespec,"%s\\%s",path,d.cFileName);
				addDir(filespec,extensions,1);
			}

      if (!(d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
        char *ext=(char *)Std::extension(d.cFileName);
        if (ext[0])
        {
          if (!stricmp(ext,"lnk"))
          {
            char temp2[MAX_PATH];
            char thisf[MAX_PATH];
            wsprintf(thisf,"%s\\%s",path,d.cFileName);
            if (ResolveShortCut(api->main_getRootWnd()->gethWnd(),thisf,temp2))
            {
              HANDLE h2;
              WIN32_FIND_DATA d2;
              if (strstr(temp2,"://")) addFile(temp2);
              else
              {
                h2=FindFirstFile(temp2,&d2);
                if (h2)
                {
                  if (!(d2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                  {
                    const char *t=extensions;
                    char *s=(char *)Std::extension(d2.cFileName);
                    while (*t)
                    {
                      if (!STRICMP(s,t))
                      {
                        addFile(temp2);
                        break;
                      }
                      t+=STRLEN(t)+1;
                    }
                  }
                  else if (recurse==1)
                  {
                    addDir(temp2,extensions,2);
                  }
                  FindClose(h2);
                }
              }
            }
          }
          else // !shortcut
          {
            const char *a=extensions;
			      wsprintf(filespec,"%s\\%s",path,d.cFileName);
            while (*a) 
            {
              if (!stricmp(a,ext)) 
              {
     		  addFile(filespec);
                  break;
              }
              a+=strlen(a)+1;
            }
          }
        }
      }
		} while (FindNextFile(i,&d));
		FindClose(i);
  }  
#endif
}

int OpenDirectory::runSelector(RootWnd *parWnd, int type, int allow_multiple, const char *ident, const char *default_dir) {

  cleanFiles();

  files.setAutoSort(false);

  directory = default_dir;

  String tname;
  if (ident != NULL) {
    tname.printf("Recent directories/OpenFile/%s", ident);
    char dir[WA_MAX_PATH]="";
    WASABI_API_CONFIG->getStringPublic(tname, dir, WA_MAX_PATH, "");
    if (*dir) directory = dir;
  }

  // save off whatever dir we're in
  char curdir[MAX_PATH];
  Std::getCurDir(curdir, MAX_PATH);
  if (!directory.isempty()) Std::setCurDir(directory);
  
  ChooseDir cd(parWnd, TRUE);
//CUT  char buf[WA_MAX_PATH];
  int r = cd.choose(_("Open Directory"));

  if (r) {
    // record new dir if successful for next time
    api->setStringPublic(tname, cd.getResult());
    directory = cd.getResult();

    // save recursion status
    recurseSubDirs = cd.getRecursiveSelected();

    // parse extensions list
    const char *a=api->core_getSupportedExtensions();
    while (*a) {
      exts.addItem(a);
      while (*a) a++;	// skip to next extension
      a++;
    }

    // get the files from the directory
    addDir(cd.getResult());
  }

  // restore old dir
  if (!directory.isempty()) Std::setCurDir(curdir);

  files.setAutoSort(true);

  exts.removeAll();

  return r;
}

void OpenDirectory::addFile(const char *filename) {
  String *fn = new String;
  fn->printf("file:%s", filename);
  files.addItem(fn);
}

void OpenDirectory::cleanFiles() {
  files.deleteAll();
}
