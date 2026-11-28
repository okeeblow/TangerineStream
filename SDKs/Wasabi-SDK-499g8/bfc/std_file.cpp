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

#include "std_file.h"

#ifdef WIN32
#include <shellapi.h>	// for ShellExecute
#endif

#define TMPNAME_PREFIX "WT"

#if UTF8
#ifdef WANT_UTF8_WARNINGS
#pragma CHAT("mig", "all", "UTF8 is enabled in std_file.cpp -- Things might be screwy till it's all debugged?")
#endif
# include <bfc/encodedstr.h>
#endif

#ifndef _NOSTUDIO
#include <bfc/parse/pathparse.h>
#include <studio/api.h>

#undef fopen
#undef fclose
#undef fseek
#undef ftell
#undef fread
#undef fwrite
#undef fgets
#undef fprintf
static PtrList<void> fileReaders;

#ifdef LINUX
FILE *FOPEN_NOCASE( const char *filename, const char *mode ) {
  FILE *ret;

  ret = fopen( filename, mode );

  if ( ! ret ) {
    // Try to mix the case up...
    String infile;
    String curdir;
    if ( filename[0] == DIRCHAR ) {
      infile = filename + 1;
      curdir = "/";
    } else {
      infile = filename;
      curdir = ".";
    }
    char *dir = strtok( (char *)infile.getValue(), DIRCHARSTR );
    int found = 1;

    while ( found && dir && curdir != "" ) {
      found = 0;

      char *next = strtok( NULL, DIRCHARSTR );
      ReadDir rd( curdir );

      while( rd.next() ) {
	if ( ! STRICMP( rd.getFilename(), dir ) ) {
	  if ( next && ! rd.isDir() ) {
	    curdir = "";
	  } else {
	    curdir += DIRCHARSTR;
	    curdir += rd.getFilename();
	  }

	  found = 1;
	  break;
	}
      }

      dir = next;
    }

    if ( found )
      ret = fopen( curdir, mode );
  }

  return ret;
}
#endif

FILE *FOPEN(const char *filename,const char *mode)
{
#ifndef WASABI_COMPILE_FILEREADER
  return fopen(filename, mode);
#else
  if (!STRNINCMP(filename, "file:")) filename += STRLEN("file:");
  FILE *ret = NULL;

#if UTF8
  if (Std::encodingSupportedByOS(SvcStrCnv::UTF16)) {
    // CODE FOR NT COMPATIBLE OS'S
    EncodedStr fname16enc;
    int retcode = fname16enc.convertFromUTF8(SvcStrCnv::UTF16, String(filename));
    wchar_t *fname16 = static_cast<wchar_t *>(fname16enc.getEncodedBuffer());
    EncodedStr mode16enc;
    mode16enc.convertFromUTF8(SvcStrCnv::UTF16, String(mode));
    wchar_t *mode16 = static_cast<wchar_t *>(mode16enc.getEncodedBuffer());
    if (fname16 != NULL) {
      ret = _wfopen(fname16,mode16);
    } else if (retcode == SvcStrCnv::ERROR_UNAVAILABLE) {
      // If we fail conversion, the service might not be available yet.
      // so we'll try opening the file and pray that it's ascii7.
      ret=fopen(filename,mode);
    }
  } else {
    // CODE FOR 9x COMPATIBLE OS'S
    EncodedStr fnameOSenc;
    int retcode = fnameOSenc.convertFromUTF8(SvcStrCnv::OSNATIVE, String(filename));
    char *fnameOS = static_cast<char *>(fnameOSenc.getEncodedBuffer());
    if (fnameOS != NULL) {
      ret=fopen(fnameOS,mode);
    } else if (retcode == SvcStrCnv::ERROR_UNAVAILABLE) {
      // If we fail conversion, conversion might not be available yet.
      // so we'll try opening the file and pray that it's ascii7.
      ret=fopen(filename,mode);
    }
  }
#else
  ret=fopen(filename,mode);
#endif

  if (ret) return ret;

  // File not found... try to open it with the file readers
  // but before that, resolve ".." in path so zip can find it

  const char *rFilename = filename;
  char str[WA_MAX_PATH]="";

  if (STRSTR(filename, "..")) {
    PathParser pp(filename);
    for (int i=0;i<pp.getNumStrings();i++) {
      if (STREQL(pp.enumString(i), "..")) {
        PathParser pp2(str);
        if (pp2.getNumStrings() <= 0) return NULL;
        ASSERTPR(pp2.getNumStrings() > 0, "we don't handle this right, and I'm not sure how to fix it because I'm not sure what the code should do with a leading .. --BU");
        int l = STRLEN(pp2.enumString(pp2.getNumStrings()-1));
        str[STRLEN(str)-l-1]=0;
        continue;
      }
      if (STREQL(pp.enumString(i), ".")) 
        continue;
      STRCAT(str, pp.enumString(i));
      STRCAT(str, "/");
    }
    str[STRLEN(str)-1]=0;
    rFilename = str;
  }

  if (WASABI_API_FILE && (ret=(FILE *)WASABI_API_FILE->fileOpen(rFilename,mode))) {
    fileReaders.addItem((void *)ret);
    return ret;
  }

	// File still not found ...
#ifdef _DEBUG
	char tmp[512];
	wsprintf(tmp,"Warning: can't open %s\n",filename);
	OutputDebugString(tmp);
#endif
  return 0;
#endif
}

int FCLOSE(FILE *stream) {
#ifdef WASABI_COMPILE_FILEREADER
  if (fileReaders.searchItem((void *)stream) != -1) {
    fileReaders.removeItem((void *)stream);
    WASABI_API_FILE->fileClose((void *)stream);
    return 0;
  } 
#endif
  return fclose(stream);
}

int FSEEK(FILE *stream, long offset, int origin) {
#ifdef WASABI_COMPILE_FILEREADER
  if (fileReaders.searchItem((void *)stream)!=-1)
    return WASABI_API_FILE->fileSeek(offset, origin,(void *)stream);
#endif
  return fseek(stream,offset,origin);
}

long FTELL(FILE *stream) {
#ifdef WASABI_COMPILE_FILEREADER
  return ftell(stream);
  if (fileReaders.searchItem((void *)stream)!=-1)
    return WASABI_API_FILE->fileTell((void *)stream);
#endif
  return ftell(stream);
}

size_t FREAD(void *buffer, size_t size, size_t count, FILE *stream) {
#ifdef WASABI_COMPILE_FILEREADER
  if (fileReaders.searchItem((void *)stream)!=-1)
    return WASABI_API_FILE->fileRead(buffer,size*count,(void *)stream);
#endif
  return fread(buffer,size,count,stream);
}

size_t FWRITE(const void *buffer, size_t size, size_t count, FILE *stream) {
#ifdef WASABI_COMPILE_FILEREADER
  if (fileReaders.searchItem((void *)stream) != -1)
    return WASABI_API_FILE->fileWrite(buffer,size*count,(void *)stream);
#endif
  return fwrite(buffer,size,count,stream);
}

int FGETSIZE(FILE *stream) {
#ifdef WASABI_COMPILE_FILEREADER
  if (fileReaders.searchItem((void *)stream)!=-1)
    return WASABI_API_FILE->fileGetFileSize((void *)stream);
#endif
  int p = ftell(stream);
  fseek(stream, 0, SEEK_END);
  int s = ftell(stream);
  fseek(stream, p, SEEK_SET);
  return s;
}

char *FGETS(char *string, int n, FILE *stream) {
#ifdef WASABI_COMPILE_FILEREADER
  if (fileReaders.searchItem((void *)stream) != -1) {
    char c;
    char *p=string;
    for(int i=0;i<(n-1);i++) {
      if(!WASABI_API_FILE->fileRead(&c,1,stream)) {
        if(!i) return NULL;
        break;
      }
      if(c==0x0d) continue;
      if(c==0x0a) break;
      *p++=c;
    }
    *p=0;
    return string;
  }
#endif
  return fgets(string,n,stream);
}

int FPRINTF(FILE *stream, const char *format , ...) {
  int ret;
  va_list args;
  va_start (args, format);	
#ifdef WASABI_COMPILE_FILEREADER
  if (fileReaders.searchItem((void *)stream) != -1) {
    String p;
    ret = p.vsprintf(format, args);
    FWRITE(p.v(), p.len(), 1, stream);
  } else
#endif
    ret = vfprintf(stream, format, args); //real stdio
  va_end (args);
  return ret;
}

const char *TMPNAM2(char *str, int val) {
#ifdef WIN32
  String tempPath8;
  static char tempName8[WA_MAX_PATH];

#if UTF8
  int failed = 0;
  if (Std::encodingSupportedByOS(SvcStrCnv::UTF16)) {
    // CODE FOR NT COMPATIBLE OS'S
    wchar_t tempPath16[WA_MAX_PATH];
    wchar_t tempName16[WA_MAX_PATH];
    GetTempPathW(WA_MAX_PATH, tempPath16);
    if (tempPath16[WSTRLEN(tempPath16)-1] != '\\') {
      WSTRCAT(tempPath16, L"\\");
    }
    GetTempFileNameW(tempPath16, L"WT", val, tempName16);
    // Convert
    String tempN8;
    EncodedStr tempPath16enc(SvcStrCnv::UTF16, tempPath16, WSTRLEN(tempPath16) + 1, 0); // you must give the size PLUS the null term if any.
    tempPath16enc.convertToUTF8(tempPath8);
    EncodedStr tempName16enc(SvcStrCnv::UTF16, tempName16, WSTRLEN(tempName16) + 1, 0); // you must give the size PLUS the null term if any.
    int retcode = tempName16enc.convertToUTF8(tempN8);
    if (retcode > 0) {
      // Copy to the static retbuf.
      STRNCPY(tempName8, tempN8, WA_MAX_PATH);
    } else if (retcode == SvcStrCnv::ERROR_UNAVAILABLE) {
      failed = 1;
    }
  } else {
    // CODE FOR 9x COMPATIBLE OS'S
    char tempPathOS[WA_MAX_PATH];
    char tempNameOS[WA_MAX_PATH];
    GetTempPath(WA_MAX_PATH, tempPathOS);
    if (tempPathOS[STRLEN(tempPathOS)-1] != '\\') {
      STRCAT(tempPathOS, "\\");
    }
    GetTempFileName(tempPathOS, TMPNAME_PREFIX, val, tempNameOS);
    // Convert
    String tempN8;
    EncodedStr tempPathOSenc(SvcStrCnv::OSNATIVE, tempPathOS, STRLEN(tempPathOS) + 1, 0); // you must give the size PLUS the null term if any.
    tempPathOSenc.convertToUTF8(tempPath8);
    EncodedStr tempNameOSenc(SvcStrCnv::OSNATIVE, tempNameOS, STRLEN(tempNameOS) + 1, 0); // you must give the size PLUS the null term if any.
    int retcode = tempNameOSenc.convertToUTF8(tempN8);
    if (retcode > 0) {
      // Copy to the static retbuf.
      STRNCPY(tempName8, tempN8, WA_MAX_PATH);
    } else if (retcode == SvcStrCnv::ERROR_UNAVAILABLE) {
      failed = 1;
    }
  }

  // Drop back and punt.
  if (failed) {
    char tempPath[WA_MAX_PATH];
    static char tempName[WA_MAX_PATH];
    GetTempPath(WA_MAX_PATH, tempPath);
    if (tempPath[STRLEN(tempPath)-1] != '\\')
      STRCAT(tempPath, "\\");
    GetTempFileName(tempPath,TMPNAME_PREFIX,val,tempName);
    STRNCPY(tempName8, tempName, WA_MAX_PATH);
  }

#else // The code that was there before.
  char tempPath[WA_MAX_PATH];
  static char tempName[WA_MAX_PATH];
  GetTempPath(WA_MAX_PATH, tempPath);
  if (tempPath[STRLEN(tempPath)-1] != '\\')
    STRCAT(tempPath, "\\");
  GetTempFileName(tempPath,TMPNAME_PREFIX,val,tempName);
#endif

  if (str) {
#if UTF8
    STRCPY(str, tempName8);
#else
    STRCPY(str, tempName);
#endif
    return str;
  } else {
#if UTF8
    return tempName8;
#else
    return tempName;
#endif
  }
#else
  return tmpnam(str);
#endif
}

const char *TMPNAM(char *string) {
  return TMPNAM2(string, 0);
}

int FEXISTS(const char *filename) {
#ifdef WASABI_COMPILE_FILEREADER
  return WASABI_API_FILE->fileExists(filename);
#else
  return (access(filename, 0) == 0);
#endif
}

int UNLINK(const char *filename) {
#ifdef WASABI_COMPILE_FILEREADER
  return FDELETE(filename);
#else
  return unlink(filename);
#endif
}

int ACCESS(const char *filename, int mode) {
  return _access(filename, mode);
}

int FDELETE(const char *filename, int permanently) {
#ifdef WASABI_COMPILE_FILEREADER
  if (permanently)
    return WASABI_API_FILE->fileRemove(filename);
  else
    return WASABI_API_FILE->fileRemoveUndoable(filename);
#else
  return UNLINK(filename);
#endif
}

int MOVEFILE(const char *filename, const char *destfilename) {
#ifdef WASABI_COMPILE_FILEREADER
  return WASABI_API_FILE->fileMove(filename, destfilename);
#else
#ifdef WIN32
  return MoveFile(filename, destfilename);
#else
#error port me!
#endif
#endif
}

int COPYFILE(const char *filename, const char *destfilename) {
#ifdef WIN32
//TODO: move the following code into the file readers
//#pragma CHAT("bas", "rOn", "feel free to move this into the readers")
  SHFILEOPSTRUCT s;
  ZERO(s);
  s.hwnd = NULL;

  s.wFunc = FO_COPY;

  char from[WA_MAX_PATH];
  STRCPY(from, filename);
  from[STRLEN(from)+1] = 0;	// extra null
  s.pFrom = from;

  char to[WA_MAX_PATH];
  STRCPY(to, destfilename);
  to[STRLEN(to)+1] = 0;	// extra null
  s.pTo = to;

  s.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;

  int r = SHFileOperation(&s);
  
  if (s.fAnyOperationsAborted) return 0;
  return !r;
#else
  FILE *fin = FOPEN( filename, "rb" );
  FILE *fout = FOPEN( destfilename, "wb" );

  if ( !fin || ! fout )
    return 0;

  int bytes;
  char buf[32768];
  while( bytes = FREAD( buf, 1, 32768, fin ) ) {
    FWRITE( buf, 1, bytes, fout );
  }

  FCLOSE( fin );
  FCLOSE( fout );

  return 1;
#endif
}

#ifdef WIN32
#include <shlobj.h>
#include <shellapi.h>

static HRESULT ResolveShortCut(LPCSTR pszShortcutFile, LPSTR pszPath)
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
       hres = psl->Resolve(HWND_DESKTOP, SLR_ANY_MATCH);
       if (SUCCEEDED(hres)) {
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

int StdFile::resolveShortcut(const char *filename, char *destfilename) {
#ifdef WIN32
  return ResolveShortCut(filename, destfilename);
#else
#error port me!
#endif
}

#endif // ndef _NOSTUDIO
