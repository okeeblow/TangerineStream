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
#include "std.h"
#include <wasabicfg.h>
#include <bfc/parse/pathparse.h>

#if !defined(WIN32) && !defined(LINUX)
#error port me!
#endif

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN

#include <shlobj.h>
#include <multimon.h>
#if !defined(HMONITOR_DECLARED) && (WINVER < 0x500)
DECLARE_HANDLE(HMONITOR);
#define HMONITOR_DECLARED
#endif

#include <objbase.h>
#include <shellapi.h>	// for ShellExecute
#include <math.h>
#include <mbctype.h>

#include <sys/stat.h>

#ifndef SPI_GETWHEELSCROLLLINES
#  define SPI_GETWHEELSCROLLLINES	104
#endif

#ifndef SPI_GETLISTBOXSMOOTHSCROLLING
#  define SPI_GETLISTBOXSMOOTHSCROLLING	0x1006
#endif

#endif	// WIN32

#include <studio/assert.h>

#include <bfc/ptrlist.h>

#if UTF8
#ifdef WANT_UTF8_WARNINGS
#pragma CHAT("mig", "all", "UTF8 is enabled in std.cpp -- Things might be screwy till it's all debugged?")
#endif
# include <bfc/encodedstr.h>
#endif

#include "time.h"	// CUT if possible

#define USE_MALLOC
//#define COUNTUSAGE

#ifdef USE_MALLOC
#include <malloc.h>
#endif

#ifdef USE_MEM_CHECK
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifdef COUNTUSAGE
static totalMem=0;
#endif

void *DO_MALLOC(int size EXTRA_INFO) {
  ASSERT(size > 0);
  if (size <= 0) return NULL;

#ifdef COUNTUSAGE
  totalMem+=size;
#endif

#ifndef USE_MEM_CHECK
 #ifndef USE_MALLOC
  return GlobalAlloc(GPTR, size);
 #else
  void *p = malloc(size);
  if (p != NULL) MEMSET(p, 0, size);//BU added in response to talkback info
  return p;
 #endif
#else
 #ifndef USE_MALLOC
  return GlobalAlloc(GPTR, size , file, line);
 #else
  void *p = _malloc_dbg(size, _NORMAL_BLOCK, file, line);
  memset(p, 0, size);
  return p;
 #endif
#endif
}
              
void *DO_CALLOC(int records, int recordsize EXTRA_INFO) {
  ASSERT(records > 0 && recordsize > 0);
  return DO_MALLOC(records * recordsize EXTRA_PARAMS);
}

void DO_FREE(void *ptr EXTRA_INFO) {
  if (ptr == NULL) return;

#ifdef COUNTUSAGE
 #ifndef USE_MALLOC
  totalMem-=GlobalSize(ptr);
 #else
  totalMem-=_msize(ptr);
 #endif
#endif

#ifndef USE_MEM_CHECK
 #ifndef USE_MALLOC
  GlobalFree(ptr);
 #else
  free(ptr);
 #endif
#else
 #ifndef USE_MALLOC
  GlobalFree(ptr, file, line); // FG> same as GlobalAlloc here, i'd like a way to translate my define inside the macro
 #else
  _free_dbg(ptr, _NORMAL_BLOCK);
 #endif
#endif
}

// Note: MEMCPY allows dest and src to overlap
void MEMCPY(void *dest, const void *src, int n) {
  ASSERT(dest != NULL);
  ASSERT(src != NULL);
  ASSERT(n >= 0);
  memmove(dest, src, n);
}

void *DO_MEMDUP(const void *src, int n EXTRA_INFO) {
  void *ret;
  ASSERT(n >= 0);
  if (src == NULL || n == 0) return NULL;
  ret = DO_MALLOC(n EXTRA_PARAMS);
  if (ret == NULL) return NULL;
  MEMCPY(ret, src, n);
  return ret;
}

#define FAKE_REALLOC

//PORTME: BU> umm... tried to merge w/ linux ver but doesn't look right
void *DO_REALLOC(void *ptr, int size EXTRA_INFO) {
  ASSERT(size >= 0);
  if (size == 0) {
#ifdef USE_MEM_CHECK
    DO_FREE(ptr, file, line);
#else
    DO_FREE(ptr);
#endif
    return NULL;
  }

#ifdef USE_MEM_CHECK
  if(ptr == NULL) return DO_MALLOC(size, file, line);
#else
  if(ptr == NULL) return DO_MALLOC(size);
#endif
  void *r=NULL; 

#ifdef COUNTUSAGE
 #ifndef USE_MALLOC
  totalMem-=GlobalSize(ptr);
 #else
  totalMem-=_msize(ptr);
 #endif
#endif

#ifndef USE_MEM_CHECK
 #ifndef USE_MALLOC
  r = GlobalReAlloc((HGLOBAL)ptr, size, GMEM_ZEROINIT);
 #else
  int oldsize = _msize(ptr);
  r = realloc(ptr, size);
 #endif // use_malloc
#else // mem_spy
 #ifndef USE_MALLOC
  r = GlobalReAlloc((HGLOBAL)ptr, size, file, line);
 #else
  int oldsize = _msize(ptr);
  r = _realloc_dbg(ptr, size, _NORMAL_BLOCK, file, line);
 #endif
#endif

  if (!r) { // realloc failed
    void *newblock = MALLOC(size);
#ifndef USE_MALLOC
    int s = GlobalSize(ptr);
#else
    int s = _msize(ptr);
#endif
    MEMCPY(newblock, ptr, MIN(s, size));
    FREE(ptr);
    r = newblock;
  } 

#ifdef COUNTUSAGE
 #ifndef USE_MALLOC
  totalMem+=size;
 #else
  totalMem+=size;
 #endif
#endif

  #ifdef USE_MALLOC
   if (size > oldsize)
    MEMZERO((char *)r+oldsize, size-oldsize);
  #endif
  return r;
}

#if 0
int TOUPPER(int c) {
  if (c >= 'a' && c <= 'z') c = (c - 'a') + 'A';
  return c;
}

int TOLOWER(int c) {
  if (c >= 'A' && c <= 'Z') c = (c - 'A') + 'a';
  return c;
}

int ISDIGIT(int c) {
  return (c >= '0' && c <= '9');
}

int ISALPHA(int c) {
  return (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z');
}
#endif

char *DO_STRDUP(const char *ptr EXTRA_INFO) {
  char *ret;
  if (ptr == NULL) return NULL;
  ret = (char *)DO_MALLOC(STRLEN(ptr)+1 EXTRA_PARAMS);
  if (ret != NULL) {
    STRCPY(ret, ptr);
  }
  return ret;
}

int STRLEN(const char *str) {
  ASSERT(str != NULL);
  return strlen(str);
}

int STRCMP(const char *str1, const char *str2) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  return strcmp(str1, str2);
}

int STRCMPSAFE(const char *str1, const char *str2, const char *defval1, const char *defval2) {
  if (str1 == NULL) str1 = defval1;
  if (str2 == NULL) str2 = defval2;
  return STRCMP(str1, str2);
}

int STRICMP(const char *str1, const char *str2) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  return lstrcmpi(str1, str2);
}

int STRICMPSAFE(const char *str1, const char *str2, const char *defval1, const char *defval2) {
  if (str1 == NULL) str1 = defval1;
  if (str2 == NULL) str2 = defval2;
  return STRICMP(str1, str2);
}

int STREQLSAFE(const char *str1, const char *str2, const char *defval1, const char *defval2) {
  return !STRCMPSAFE(str1, str2, defval1, defval2);
}

int STRCASEEQLSAFE(const char *str1, const char *str2, const char *defval1, const char *defval2) {
  return !STRICMPSAFE(str1, str2, defval1, defval2);
}

int STRNCMP(const char *str1, const char *str2, int l) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  return strncmp(str1, str2, l);
}

int STRNNCMP(const char *str1, const char *str2) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  return STRNCMP(str1, str2, STRLEN(str2));
}

//FG> sorry brennan, this need to not be a const :)
void STRTOUPPER(char *p) {
  while (*p) {
    *p = TOUPPER(*p);
    p++;
  }
}

void STRTOLOWER(char *p) {
  while (*p) {
    *p = TOLOWER(*p);
    p++;
  }
}

int STRNICMP(const char *str1, const char *str2, int l) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);

  ASSERT(l>=0);
  while (TOUPPER(*str1) == TOUPPER(*str2) && *str1 != 0 && *str2 != 0 && l--)
    str1++, str2++;
  if (l == 0) return 0;
  return (*str2 - *str1);
}

int STRNINCMP(const char *str1, const char *str2) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  return STRNICMP(str1, str2, STRLEN(str2));
}

int STRNCASEEQL(const char *str1, const char *str2, int l) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  return STRNICMP(str1, str2, l) == 0;
}

int STREQL(const char *str1, const char *str2) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  return (strcmp(str1, str2) == 0);
}

int STRCASEEQL(const char *str1, const char *str2) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
#ifdef WIN32
  return (lstrcmpi(str1, str2)==0);
#else
  return (stricmp(str1, str2)==0);
#endif
}

int STRISNUM(const char *p) {
  for (; *p; p++)
    if (!ISDIGIT(*p)) return 0;
  return 1;
}

static int TOUPPERANDSLASH(int a) {
  if (a=='\\')
    a = '/';
  else
    a = TOUPPER(a);
  return a;	
}

int PATHEQL(const char *str1, const char *str2) {
  if (str1 == NULL) {
    if (str2 == NULL) return TRUE;
    return FALSE;
  }
  while (TOUPPERANDSLASH(*str1) == TOUPPERANDSLASH(*str2) && *str1 != 0 && *str2 != 0) str1++, str2++;
  return *str1 == *str2;
}

char *STRSTR(const char *str1, const char *str2) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  ASSERT(str1 != str2);
  return strstr(str1, str2);
}

char *STRCASESTR(const char *str1, const char *str2) {
  ASSERT(str1!=NULL);
  ASSERT(str2!=NULL);
  char *p;
  int len = STRLEN(str2);
  for (p = (char *)str1; *p; p++) {
    if (STRNCASEEQL(p, str2, len)) return p;
  }
  return NULL;
}

int STRCASERPL(char *s, const char *w, const char *r, int maxsize) {

  int rl = STRLEN(r);
  int wl = STRLEN(w);
  int gain = rl - wl;
  int i=0;
  char *o = s;

  while (1) {
    char *f = STRCASESTR(o, w);
    if (!f) break;
    int pos = s - f;
    if (pos + wl + gain >= maxsize) break;
    if (gain < 0) {
      MEMCPY(f+wl+gain, f+wl, maxsize - pos - wl);
      s[maxsize-1] = 0;
    }
    else if (gain > 0) {
      MEMCPY(f+wl+gain, f+wl, maxsize - pos - wl - gain);
      s[maxsize-1] = 0;
    }
    i++;    
    MEMCPY(f, r, rl);
    o = f+rl;
  }
  return i;
}

void STRCPY(char *dest, const char *src) {
  ASSERT(dest != NULL);
  ASSERT(src != NULL);
  ASSERT(dest != src);
  strcpy(dest, src);
//INLINE
}

void STRNCPY(char *dest, const char *src, int maxchar) {
  ASSERT(dest != NULL);
  ASSERT(src != NULL);
  strncpy(dest, src, maxchar);
//INLINE
}

char *STRCHR(const char *str, int c) {
  ASSERT(str != NULL);
  return strchr(str, c);
//INLINE
}

char *STRRCHR(const char *str, int c) {
  ASSERT(str != NULL);
  const char *p = str+STRLEN(str)-1;
  while (*p && p >= str) {
    if (*p == c) return (char *)p;
    p--;
  }
  return NULL;
}

void STRCAT(char *dest, const char *append) {
  ASSERT(dest != NULL);
  ASSERT(append != NULL);
  ASSERT(dest != append);
  strcat(dest, append);
}

unsigned long STRTOUL(const char *s, char **p, int rx) {
  ASSERT(s != NULL);
  ASSERT(p != NULL);
  return strtoul(s,p,rx);
}

int WSTRLEN(const wchar_t *str16) {
  if (!str16) return 0;
#ifdef WIN32
  return lstrlenW(str16);
#else
  int r = 0;
  while (*str++ != 0) r++;
  return r;
#endif
}

// Got bored trying to dup all my own 8's to 16's
wchar_t *DO_WSTRDUP(const char *ptr8 EXTRA_INFO) {
  wchar_t *ret;
  if (ptr8 == NULL) return NULL;
  int size8 = STRLEN(ptr8);
  ret = (wchar_t *)DO_MALLOC((size8 + 1) * sizeof(wchar_t) EXTRA_PARAMS);
  if (ret != NULL) {
    int i;
    for (i = 0; i < size8; i++) {
      // thou shalt not sign extend.
      ret[i] = (ptr8[i]) & 0xFF;
    }
  }
  return ret;
}

void WSTRCAT(wchar_t *dest, const wchar_t *append) {
  ASSERT(dest != NULL);
  ASSERT(append != NULL);
  ASSERT(dest != append);
#ifdef WIN32
  lstrcatW(dest, append);
#else
  OutputDebugString( "portme -- Std, WSTRCAT\n" );
#error port me
#endif
}

void WSTRCAT7(wchar_t *dest, const char *append) {
  ASSERT(dest != NULL);
  ASSERT(append != NULL);
  wchar_t *dupe = WSTRDUP(append);
  WSTRCAT(dest, dupe);
  FREE(dupe);
}

int WSTRCMP(const wchar_t *a, const wchar_t *b) {
  ASSERT(a != NULL);
  ASSERT(b != NULL);
#ifdef WIN32
  return lstrcmpW(a, b);
#else
#error port me
  OutputDebugString( "portme -- Std, WSTRCMP\n" );
  return 0;
#endif
}

int WSTRCMP7(const wchar_t *a, const char *b) {
  ASSERT(a != NULL);
  ASSERT(b != NULL);
  wchar_t *dupe = WSTRDUP(b);
  int retval = WSTRCMP(a, dupe);
  FREE(dupe);
  return retval;
}

static Std blep;
static int rand_inited;

static double divider=0.;

Std::Std() {
#ifdef WIN32
  LARGE_INTEGER ll;
  QueryPerformanceFrequency(&ll);
  divider = (double)ll.QuadPart;
#endif
}

int Std::keyDown(int code) {
#ifdef WIN32
  return !!(GetKeyState(code) & 0x8000);
#elif defined(LINUX)
  if ( k == MK_RBUTTON || k == MK_LBUTTON ) {
    Window t1, t2; 
    int rx, ry, wx, wy;
    unsigned int buttons;

    XQueryPointer( Linux::getDisplay(), Linux::RootWin(), &t1, &t2, 
		   &rx, &ry, &wx, &wy, &buttons );

    if ( k == MK_RBUTTON )
      return buttons & Button3Mask;
    else
      return buttons & Button1Mask;
  }

  int code1 = XKeysymToKeycode( Linux::getDisplay(), k & 0xFFFF );
  int code2 = XKeysymToKeycode( Linux::getDisplay(), (k>>16) & 0xFFFF );

  char keys_return[32];
  XQueryKeymap( Linux::getDisplay(), keys_return );
  
  if ( code1 && code2 )
    return (keys_return[ (code1 >> 3) & 31 ] & (1 << (code1 & 7))) ||
      (keys_return[ (code2 >> 3) & 31 ] & (1 << (code2 & 7)));
  
  return (keys_return[ (code1 >> 3) & 31 ] & (1 << (code1 & 7)));
#else
#error port me!
#endif
}

void Std::getMousePos(POINT *p) {
  ASSERT(p != NULL);
#ifdef WIN32
  GetCursorPos(p);
#else
  Window w1, w2;
  int a, b;
  unsigned int c;
  int x, y;

  XQueryPointer( Linux::getDisplay(), Linux::RootWin(), &w1, &w2,
		 &x, &y, &a, &b, &c );

  p->x = x;
  p->y = y;
#endif
}

void Std::getMousePos(int *x, int *y) {
  POINT p;
  getMousePos(&p);
  if (x != NULL) *x = p.x;
  if (y != NULL) *y = p.y;
}

void Std::getMousePos(long *x, long *y) {
  getMousePos((int *)x, (int *)y);
}

void Std::setMousePos(POINT *p) {
  ASSERT(p != NULL);
#ifdef WIN32
  SetCursorPos(p->x, p->y);
#else
  POINT p2;
  getMousePos( &p2 );

  XWarpPointer( Linux::getDisplay(), None, None, 
		0, 0, 1, 1, p->x - p2.x, p->y - p2.y );
#endif
}

void Std::setMousePos(int x, int y) {
  POINT p={x,y};
  setMousePos(&p);
}

void Std::getViewport(RECT *r, POINT *p, int full) {
  getViewport(r, p, NULL, NULL, full);
}

void Std::getViewport(RECT *r, RECT *sr, int full) {
  getViewport(r, NULL, sr, NULL, full);
}

void Std::getViewport(RECT *r, HWND wnd, int full) {
  getViewport(r, NULL, NULL, wnd, full);
}

void Std::getViewport(RECT *r, POINT *p, RECT *sr, HWND wnd, int full) {
  ASSERT(r != NULL);
#ifdef WIN32
  if (p || sr || wnd) {
    HINSTANCE h=LoadLibrary("user32.dll");
    if (h) {
      HMONITOR (WINAPI *Mfp)(POINT pt, DWORD dwFlags) = (HMONITOR (WINAPI *)(POINT,DWORD)) GetProcAddress(h,"MonitorFromPoint");
      HMONITOR (WINAPI *Mfr)(LPCRECT lpcr, DWORD dwFlags) = (HMONITOR (WINAPI *)(LPCRECT, DWORD)) GetProcAddress(h, "MonitorFromRect");
      HMONITOR (WINAPI *Mfw)(HWND wnd, DWORD dwFlags)=(HMONITOR (WINAPI *)(HWND, DWORD)) GetProcAddress(h, "MonitorFromWindow");
      BOOL (WINAPI *Gmi)(HMONITOR mon, LPMONITORINFO lpmi) = (BOOL (WINAPI *)(HMONITOR,LPMONITORINFO)) GetProcAddress(h,"GetMonitorInfoA");    
      if (Mfp && Mfr && Mfw && Gmi) {
        HMONITOR hm;
        if (p)
          hm=Mfp(*p,MONITOR_DEFAULTTONULL);
        else if (sr)
          hm=Mfr(sr,MONITOR_DEFAULTTONULL);
        else if (wnd)
          hm=Mfw(wnd,MONITOR_DEFAULTTONULL);
        if (hm) {
          MONITORINFOEX mi;
          ZERO(mi);
          mi.cbSize=sizeof(mi);

          if (Gmi(hm,&mi)) {
            if(!full) *r=mi.rcWork;
            else *r=mi.rcMonitor;
            FreeLibrary(h);
            return;
          }          
        }
      }
      FreeLibrary(h);
    }
  }
  SystemParametersInfo(SPI_GETWORKAREA,0,r,0);
#endif
#ifdef LINUX
  r->left = r->top = 0;
  r->right = Std::getScreenWidth();
  r->bottom = Std::getScreenHeight();
#endif
}

class ViewportEnumerator {
  public:
    HMONITOR hm;
#ifdef WIN32
    int monitor_n;
#endif
    int counter;
};

#ifdef WIN32
static BOOL CALLBACK enumViewportsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
  ViewportEnumerator *ve = reinterpret_cast<ViewportEnumerator *>(dwData);
  ASSERT(ve != NULL);
  if (ve->counter == ve->monitor_n) {
    ve->hm = hMonitor;
    return FALSE;
  }
  ve->counter++;
  return TRUE;
}
#endif


int Std::pointInRect(RECT *r, POINT &p) {
  if (p.x < r->left ||
      p.x >= r->right ||
      p.y < r->top ||
      p.y >= r->bottom) return 0;
  return 1;
}

void Std::setRect(RECT *r, int left, int top, int right, int bottom) {
  r->left = left;
  r->top = top;
  r->right = right;
  r->bottom = bottom;
}

RECT Std::makeRect(int left, int top, int right, int bottom) {
  RECT r;
  r.left = left;
  r.top = top;
  r.right = right;
  r.bottom = bottom;
  return r;
}  

POINT Std::makePoint(int x, int y) {
  POINT p = { x, y };
  return p;
}

void Std::offsetRect(RECT *r, int x, int y) {
  r->left += x;
  r->right += x;
  r->top += y;
  r->bottom += y;
}

int Std::rectEqual(const RECT &a, const RECT &b) {
  return !MEMCMP(&a, &b, sizeof(RECT));
}

int Std::rectEqual(const RECT *a, const RECT *b) {
  return !MEMCMP(a, b, sizeof(RECT));
}

int Std::rectIntersect(const RECT *i1, const RECT *i2, RECT *intersection) {
  RECT out;
  out.left   = MAX( i1->left, i2->left );
  out.right  = MIN( i1->right, i2->right );
  out.top    = MAX( i1->top, i2->top );
  out.bottom = MIN( i1->bottom, i2->bottom );

  if (intersection != NULL) *intersection = out;
  return ( out.left < out.right && out.top < out.bottom );
}

void Std::srandom(unsigned int key) {
  if (key == 0) key = (unsigned int)Std::getTimeStamp();
  ::srand(key);
}

int Std::random(int max) {
//  ASSERT(max <= RAND_MAX);
  if (!rand_inited++) srandom();
  int ret = ::rand();
  if (max != RAND_MAX+1) ret %= max;
  return ret;
}

unsigned int Std::random32(unsigned int max) {
  if (!rand_inited++) Std::srandom();
  unsigned int val=0;
  for (int i = 0; i < sizeof(unsigned int); i++) {
    val <<= 8;
    val |= ((::rand()>>2) & 0xff);
  }
  if (max != 0xffffffff) val %= max;
  return val;
}

void Std::usleep(int ms) {
  Sleep(ms);
//INLINE
}

stdtimeval Std::getTimeStamp() {
  return time(NULL);
}

stdtimevalms Std::getTimeStampMS() {
#ifdef WIN32
  LARGE_INTEGER ll;
  if (!QueryPerformanceCounter(&ll)) return getTickCount() / 1000.f;
  stdtimevalms ret = (stdtimevalms)ll.QuadPart;
  return ret /= divider;
#else
  return getTickCount() / 1000.f;
#endif
}

DWORD Std::getTickCount() {
  return GetTickCount();
//INLINE
}

void Std::tolowerString(char *str) {
  while (*str++) *str = TOLOWER(*str);
}

void Std::ensureVisible(RECT *r) {
  RECT sr;
  POINT p={(r->right+r->left)/2,(r->bottom+r->top)/2};
  Std::getViewport(&sr,&p);
  int w = r->right-r->left;
  int h = r->bottom-r->top;
  if (r->bottom > sr.bottom) {
    r->bottom = sr.bottom;
    r->top = r->bottom-h;
  }
  if (r->right > sr.right) {
    r->right = sr.right;
    r->left = r->right-w;
  }
  if (r->left < sr.left) {
    r->left = sr.left;
    r->right = r->left+w;
  }
  if (r->top < sr.top) {
    r->top = sr.top;
    r->bottom = r->top+h;
  }
}

int Std::getScreenWidth() {
#ifdef WIN32
  RECT r;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
  return r.right-r.left;
#else
  return DisplayWidth(Linux::getDisplay(), Linux::getScreenNum());
#endif
}

int Std::getScreenHeight() {
#ifdef WIN32
  RECT r;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
  return r.bottom-r.top;
#else
  return DisplayHeight( Linux::getDisplay(), Linux::getScreenNum() );
#endif
}

int Std::messageBox(const char *txt, const char *title, int flags) {
#ifdef WIN32
  return MessageBox(NULL, txt, title, flags);
#else
 OutputDebugString(txt); 
#endif
}

int Std::getDoubleClickDelay() {
#ifdef WIN32
  return GetDoubleClickTime();
#else
  return 200;
#endif
}

#undef GetSystemMetrics //FG> DUH!
int Std::getDoubleClickX() {
#ifdef WIN32
  return GetSystemMetrics(SM_CYDOUBLECLK);
#else
  return 5;
#endif
}

int Std::getDoubleClickY() {
#ifdef WIN32
  return GetSystemMetrics(SM_CXDOUBLECLK);
#else
  return 5;
#endif
}

int Std::osparam_getScrollLines() {
  int ret = 3;
#ifdef WIN32
  SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &ret, 0);
#endif
  return ret;
}

int Std::osparam_getSmoothScroll() {
  int ret = 1;
#ifdef WIN32
  SystemParametersInfo(SPI_GETLISTBOXSMOOTHSCROLLING, 0, &ret, 0);
#endif
  return ret;
}

int Std::dirChar() {
#ifdef WIN32
  return '\\';
#else
  return '/';
#endif
}

const char *Std::dirCharStr() {
#ifdef WIN32
  return "\\";
#else
  return "/";
#endif
};

int Std::isDirChar(int thechar, int allow_multiple_platforms) {
  if (thechar == DIRCHAR) return 1;
  if (allow_multiple_platforms) {
#ifdef WIN32
    if (thechar == '/') return 1;
#else
    if (thechar == '\\') return 1;
#endif
  }
  return 0;
}

const char *Std::matchAllFiles() {
#ifdef WIN32
  return "*.*";
#else
  return "*";
#endif
}

const char *Std::dotDir() {
  return ".";
}

const char *Std::dotDotDir() {
  return "..";
}

int Std::isRootPath(const char *path) {
  if (path == NULL || path[0] == '\0') return 0;
  if (path[0] == DIRCHAR) return 1;
#ifdef WIN32
  // extra win32 check for drive letter i.e. c:\ 
  if (ISALPHA(path[0]) && path[1] == ':' && path[2] == DIRCHAR) return 1;
#endif
  return 0;
}

int Std::switchChar() {
#ifdef WIN32
  return '/';
#else
  return '-';
#endif
}

int Std::enumViewports(int monitor_n, RECT *r, int full) {
  ASSERT(r != NULL);
#ifdef WIN32
  HINSTANCE h=LoadLibrary("user32.dll");
  if (h) {
    BOOL (WINAPI *Gmi)(HMONITOR mon, LPMONITORINFO lpmi) = (BOOL (WINAPI *)(HMONITOR,LPMONITORINFO)) GetProcAddress(h,"GetMonitorInfoA");    
    BOOL (WINAPI *Edm)(HDC hdc, LPCRECT clip, MONITORENUMPROC proc, LPARAM dwdata) = (BOOL (WINAPI *)(HDC,LPCRECT,MONITORENUMPROC,LPARAM)) GetProcAddress(h,"EnumDisplayMonitors");    
    if (Gmi && Edm) {
      ViewportEnumerator ve;
      ve.monitor_n = monitor_n;
      ve.hm = NULL;
      ve.counter = 0;
      Edm(NULL, NULL, enumViewportsProc, reinterpret_cast<LPARAM>(&ve));
      HMONITOR hm = ve.hm;
      if (hm) {
        MONITORINFOEX mi;
        ZERO(mi);
        mi.cbSize=sizeof(mi);
        if (Gmi(hm,&mi)) {
          if(!full) *r=mi.rcWork;
          else *r=mi.rcMonitor;
          FreeLibrary(h);
          return 1;
        }          
      }
    }
    FreeLibrary(h);
  }
  SystemParametersInfo(SPI_GETWORKAREA,0,r,0);
  return 0;
#endif
#ifdef LINUX
  if ( monitor_n > 0 )
    return 0;
  r->left = r->top = 0;
  r->right = Std::getScreenWidth();
  r->bottom = Std::getScreenHeight();
  return 1;
#endif
}

const char *Std::scanstr_back(const char *str, const char *toscan, const char *defval) {
  int strl = STRLEN(str);
  const char *s=str+strl-1;
  if (strl < 1) return defval;
  if (STRLEN(toscan) < 1) return defval;
  while (1) {
    const char *t=toscan;
    while (*t) if (*t++ == *s) return s;
    t=CharPrev(str,s);
    if (t==s) return defval;
    s=t;
  }
}

const char *Std::extension(const char *fn) {
  if (fn == NULL) return NULL;
  const char *s = scanstr_back(fn,".\\/",fn-1);
  if (s < fn) return "";
  if (*s == '\\' || *s == '/') return "";
  return (s+1);
}

const char *Std::filename(const char *fn) {
  const char *s=scanstr_back(fn,"\\/",0);
  if(!s) return fn;
  return (s+1);
}

BOOL Std::isMatchPattern(const char *p) {
  while (*p) {
    switch (*p++) {
      case '?':
      case '*':
      case '[':
      case '\\':
        return TRUE;
    }
  }
  return FALSE;
}

BOOL Std::isValidMatchPattern(const char *p, int *error_type) {
  /* init error_type */
  *error_type = PATTERN_VALID;
  /* loop through pattern to EOS */
  while (*p) {
    /* determine pattern type */
    switch (*p) {
      /* check literal escape, it cannot be at end of pattern */
      case '\\':
			  if (!*++p) {
          *error_type = PATTERN_ESC;
          return FALSE;
        }
        p++;
        break;
      /* the [..] construct must be well formed */
      case '[':
        p++;
        /* if the next character is ']' then bad pattern */
        if (*p == ']') {
          *error_type = PATTERN_EMPTY;
          return FALSE;
        }
        /* if end of pattern here then bad pattern */
        if (!*p) {
          *error_type = PATTERN_CLOSE;
          return FALSE;
        }
        /* loop to end of [..] construct */
        while (*p != ']') {
          /* check for literal escape */
          if (*p == '\\') {
            p++;
            /* if end of pattern here then bad pattern */
            if (!*p++) {
              *error_type = PATTERN_ESC;
              return FALSE;
            }
          } else p++;
        /* if end of pattern here then bad pattern */
        if (!*p) {
          *error_type = PATTERN_CLOSE;
          return FALSE;
        }
        /* if this a range */
        if (*p == '-') {
          /* we must have an end of range */
					if (!*++p || *p == ']') {
            *error_type = PATTERN_RANGE;
            return FALSE;
          } else {
            /* check for literal escape */
            if (*p == '\\') p++;
            /* if end of pattern here then bad pattern */
            if (!*p++) {
              *error_type = PATTERN_ESC;
              return FALSE;
            }
          }
        }
      }
      break;
      /* all other characters are valid pattern elements */
      case '*':
      case '?':
      default:
        p++; /* "normal" character */
        break;
    }
  }
  return TRUE;
}

int Std::matche(register const char *p, register const char *t) {
  register char range_start, range_end;  /* start and end in range */

  BOOLEAN invert;             /* is this [..] or [!..] */
  BOOLEAN member_match;       /* have I matched the [..] construct? */
  BOOLEAN loop;               /* should I terminate? */

  for ( ; *p; p++, t++) {
    /* if this is the end of the text then this is the end of the match */
    if (!*t) {
      return ( *p == '*' && *++p == '\0' ) ? MATCH_VALID : MATCH_ABORT;
    }
    /* determine and react to pattern type */
    switch (*p) {
      case '?': /* single any character match */
        break;
			case '*': /* multiple any character match */
        return matche_after_star (p, t);

      /* [..] construct, single member/exclusion character match */
      case '[': {
        /* move to beginning of range */
        p++;
        /* check if this is a member match or exclusion match */
        invert = FALSE;
        if (*p == '!' || *p == '^') {
          invert = TRUE;
          p++;
        }
        /* if closing bracket here or at range start then we have a malformed pattern */
        if (*p == ']') return MATCH_PATTERN;

        member_match = FALSE;
        loop = TRUE;
        while (loop) {
          /* if end of construct then loop is done */
          if (*p == ']') {
            loop = FALSE;
            continue;
          }
          /* matching a '!', '^', '-', '\' or a ']' */
          if (*p == '\\') range_start = range_end = *++p;
          else  range_start = range_end = *p;
          /* if end of pattern then bad pattern (Missing ']') */
          if (!*p) return MATCH_PATTERN;
          /* check for range bar */
          if (*++p == '-') {
            /* get the range end */
            range_end = *++p;
            /* if end of pattern or construct then bad pattern */
            if (range_end == '\0' || range_end == ']') return MATCH_PATTERN;
					  /* special character range end */
            if (range_end == '\\') {
              range_end = *++p;
              /* if end of text then we have a bad pattern */
              if (!range_end) return MATCH_PATTERN;
            }
            /* move just beyond this range */
            p++;
          }
          /* if the text character is in range then match found.
             make sure the range letters have the proper
             relationship to one another before comparison */
          if (range_start < range_end) {
            if (*t >= range_start && *t <= range_end) {
              member_match = TRUE;
              loop = FALSE;
            }
          } else {
            if (*t >= range_end && *t <= range_start) {
              member_match = TRUE;
              loop = FALSE;
            }
          }
        }
        /* if there was a match in an exclusion set then no match */
        /* if there was no match in a member set then no match */
        if ((invert && member_match) || !(invert || member_match)) return MATCH_RANGE;
        /* if this is not an exclusion then skip the rest of the [...] construct that already matched. */
        if (member_match) {
          while (*p != ']') {
            /* bad pattern (Missing ']') */
            if (!*p) return MATCH_PATTERN;
            /* skip exact match */
            if (*p == '\\') {
              p++;
						  /* if end of text then we have a bad pattern */
              if (!*p) return MATCH_PATTERN;
            }
            /* move to next pattern char */
            p++;
					}
				}
        break;
      }
      case '\\':  /* next character is quoted and must match exactly */
        /* move pattern pointer to quoted char and fall through */
        p++;
        /* if end of text then we have a bad pattern */
        if (!*p) return MATCH_PATTERN;
        /* must match this character exactly */
      default:
        if (toupper(*p) != toupper(*t)) return MATCH_LITERAL;
    }
  }
	/* if end of text not reached then the pattern fails */
  if (*t) return MATCH_END;
  else return MATCH_VALID;
}

int Std::matche_after_star(register const char *p, register const char *t) {
  register int match = 0;
  register nextp;
  /* pass over existing ? and * in pattern */
  while ( *p == '?' || *p == '*' ) {
    /* take one char for each ? and + */
    if (*p == '?') {
      /* if end of text then no match */
      if (!*t++) return MATCH_ABORT;
    }
    /* move to next char in pattern */
    p++;
  }
  /* if end of pattern we have matched regardless of text left */
  if (!*p) return MATCH_VALID;
  /* get the next character to match which must be a literal or '[' */
  nextp = *p;
  if (nextp == '\\') {
    nextp = p[1];
    /* if end of text then we have a bad pattern */
    if (!nextp) return MATCH_PATTERN;
  }
  /* Continue until we run out of text or definite result seen */
  do {
    /* a precondition for matching is that the next character
       in the pattern match the next character in the text or that
       the next pattern char is the beginning of a range.  Increment
       text pointer as we go here */
    if (tolower(nextp) == tolower(*t) || nextp == '[') match = matche(p, t);
    /* if the end of text is reached then no match */
    if (!*t++) match = MATCH_ABORT;
  } while ( match != MATCH_VALID && match != MATCH_ABORT && match != MATCH_PATTERN);
  /* return result */
  return match;
}

BOOL Std::match(const char *p, const char *t) {
  int error_type;
  error_type = matche(p,t);
  return (error_type == MATCH_VALID ) ? TRUE : FALSE;
}

#ifndef _NOSTUDIO

int Std::getCurDir(char *str, int maxlen) {
  ASSERT(str != NULL);
#ifdef WIN32
  int retval = 0;
#if UTF8

  if (Std::encodingSupportedByOS(SvcStrCnv::UTF16)) {
    // CODE FOR NT COMPATIBLE OS'S
    wchar_t buf16[WA_MAX_PATH];
    retval = GetCurrentDirectoryW(WA_MAX_PATH, buf16);
    if (retval) {
      String buf8;
      EncodedStr buf16enc(SvcStrCnv::UTF16, buf16, retval+1, 0); // retval is size without nullchar, we must give size WITH nullchar.
      retval = buf16enc.convertToUTF8(buf8);
      if (retval > 0) {
        STRNCPY(str, buf8, maxlen);
      } else if (retval == SvcStrCnv::ERROR_UNAVAILABLE) {
        retval = GetCurrentDirectory(maxlen, str);
      } else {
        retval = 0;
        str[0]=0;
      }
    }
  } else {
    // CODE FOR 9x COMPATIBLE OS'S
    char bufOS[WA_MAX_PATH];
    retval = GetCurrentDirectory(WA_MAX_PATH, bufOS);
    if (retval) {
      String buf8;
      EncodedStr bufOSenc(SvcStrCnv::OSNATIVE, bufOS, retval+1, 0); // retval is size without nullchar, we must give size WITH nullchar.
      retval = bufOSenc.convertToUTF8(buf8);
      if (retval > 0) {
        STRNCPY(str, buf8, maxlen);
      } else if (retval == SvcStrCnv::ERROR_UNAVAILABLE) {
        retval = GetCurrentDirectory(maxlen, str);
      } else {
        retval = 0;
        str[0]=0;
      }
    }
  }

#else // The code that was there before.
  retval = GetCurrentDirectory(maxlen, str);
#endif

  return retval;

#else
  return getcwd( str, maxlen ) != NULL;
#endif
}

int Std::setCurDir(const char *str) {
  ASSERT(str != NULL);
#ifdef WIN32
  int retval = 0;
#if UTF8
  if (Std::encodingSupportedByOS(SvcStrCnv::UTF16)) {
    // CODE FOR NT COMPATIBLE OS'S
    EncodedStr dname16enc;
    int check = dname16enc.convertFromUTF8(SvcStrCnv::UTF16, String(str));
    if (check > 0) {
      wchar_t *dname16 = static_cast<wchar_t *>(dname16enc.getEncodedBuffer());
      retval = SetCurrentDirectoryW(dname16);
    } else if (check == SvcStrCnv::ERROR_UNAVAILABLE) {
      retval = SetCurrentDirectory(str);
    }
  } else {
    // CODE FOR 9x COMPATIBLE OS'S
    EncodedStr dnameOSenc;
    int check = dnameOSenc.convertFromUTF8(SvcStrCnv::OSNATIVE, String(str));
    if (check > 0) {
      char *dnameOS = static_cast<char *>(dnameOSenc.getEncodedBuffer());
      retval = SetCurrentDirectory(dnameOS);
    } else if (check == SvcStrCnv::ERROR_UNAVAILABLE) {
      retval = SetCurrentDirectory(str);
    }
  }

#else // The code that was there before.
  retval = SetCurrentDirectory(str);
#endif

  return retval;

#else
  return chdir( str );
#endif
}

int Std::getNumCPUs() {
#ifdef WIN32
  SYSTEM_INFO si;
  ZERO(si);
  GetSystemInfo(&si);
  return si.dwNumberOfProcessors;
#else
#error port me!
#endif
}

THREADID Std::getCurrentThreadId() {
#ifdef WIN32
  return GetCurrentThreadId();
#else
  return pthread_self();
#endif
}

void Std::setThreadPriority(int delta, HANDLE thread_handle) {
#ifdef WIN32
  if (thread_handle == NULL) thread_handle = GetCurrentThread();
  int v = THREAD_PRIORITY_NORMAL;
  switch (delta) {
    case -32767: v = THREAD_PRIORITY_IDLE; break;
    case -2: v = THREAD_PRIORITY_LOWEST; break;
    case -1: v = THREAD_PRIORITY_BELOW_NORMAL; break;
    case 1: v = THREAD_PRIORITY_ABOVE_NORMAL; break;
    case 2: v = THREAD_PRIORITY_HIGHEST; break;
    case 32767: v = THREAD_PRIORITY_TIME_CRITICAL; break;
  }
  SetThreadPriority(thread_handle, v);
#else
#error port me!
#endif
}

String Std::getLastErrorString(int _err) {
  String ret;
  if (_err == -1) {
#ifdef WIN32
    _err = GetLastError();
#else
#error port me!
#endif
  }
#ifdef WIN32
  char *sysbuf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, _err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *)&sysbuf, 0, NULL);
  ret = sysbuf;
  LocalFree(sysbuf);
#else
#error port me!
#endif
  return ret;
}

void Std::getTempPath(int bufsize, char *dst) {
  ASSERT(dst != NULL);
#ifdef WIN32
#if UTF8

  int failed = 0;

  String tempPath8;
  if (Std::encodingSupportedByOS(SvcStrCnv::UTF16)) {
    // CODE FOR NT COMPATIBLE OS'S
    wchar_t tempPath16[WA_MAX_PATH];
    GetTempPathW(WA_MAX_PATH, tempPath16);
    // Convert
    EncodedStr tempPath16enc(SvcStrCnv::UTF16, tempPath16, WSTRLEN(tempPath16) + 1, 0); // you must give the size PLUS the null term if any.
    failed = tempPath16enc.convertToUTF8(tempPath8);
  } else {
    // CODE FOR 9x COMPATIBLE OS'S
    char tempPathOS[WA_MAX_PATH];
    GetTempPath(WA_MAX_PATH, tempPathOS);
    // Convert
    EncodedStr tempPathOSenc(SvcStrCnv::OSNATIVE, tempPathOS, STRLEN(tempPathOS) + 1, 0); // you must give the size PLUS the null term if any.
    failed = tempPathOSenc.convertToUTF8(tempPath8);
  }

  if (failed == SvcStrCnv::ERROR_UNAVAILABLE) {
    GetTempPath(bufsize,dst);
  } else if (tempPath8.len()) {
    // Copy to the user buf.
    STRNCPY(dst, tempPath8, bufsize);
  } else {
    // ??? what do we do?  Null the string?  I guess.
    dst[0] = 0;
  }

#else // The code that was there before.
  GetTempPath(bufsize,dst);
#endif

#else
  char *tmpdir;
  struct stat st;

  if ( !(tmpdir = getenv( "TMPDIR" )) || stat( tmpdir, &st ) != 0 ) {
    tmpdir = P_tmpdir;
    if ( stat( tmpdir, &st ) != 0 ) 
      tmpdir = "/tmp";
  }

  STRNCPY( dst, tmpdir, bufsize );
  STRCAT( dst, DIRCHARSTR );
#endif
}

// gets the system font path
void Std::getFontPath(int bufsize, char *dst) {
  ASSERT(dst != NULL);
#ifdef WIN32
#if UTF8

  int failed = 0;

  String winPath8;
  if (Std::encodingSupportedByOS(SvcStrCnv::UTF16)) {
    // CODE FOR NT COMPATIBLE OS'S
    wchar_t winPath16[WA_MAX_PATH];
    SHGetSpecialFolderPathW(NULL, winPath16, CSIDL_FONTS, FALSE);
    // Convert
    EncodedStr winPath16enc(SvcStrCnv::UTF16, winPath16, WSTRLEN(winPath16) + 1, 0); // you must give the size PLUS the null term if any.
    failed = winPath16enc.convertToUTF8(winPath8);
  } else {
    // CODE FOR 9x COMPATIBLE OS'S
    char winPathOS[WA_MAX_PATH];
    SHGetSpecialFolderPath(NULL, winPathOS, CSIDL_FONTS, FALSE);
    // Convert
    EncodedStr winPathOSenc(SvcStrCnv::OSNATIVE, winPathOS, STRLEN(winPathOS) + 1, 0); // you must give the size PLUS the null term if any.
    failed = winPathOSenc.convertToUTF8(winPath8);
  }

  if (failed == SvcStrCnv::ERROR_UNAVAILABLE || winPath8.len() == 0) {
    char path[8192]="";
    SHGetSpecialFolderPath(NULL, path, CSIDL_FONTS, FALSE);
    STRNCPY(dst, path, bufsize);
  } else {
    // Copy to the user buf.
    STRNCPY(dst, winPath8, bufsize);
  }
#else // The code that was there before.
  char path[8192]="";
  SHGetSpecialFolderPath(NULL, path, CSIDL_FONTS, FALSE);
  STRNCPY(dst, path, bufsize);
#endif

#else
#error port me!
#endif

}

int default_font_scale = 100;
char default_font[256] = 
#ifdef WIN32
WASABI_DEFAULT_FONTNAME ".ttf";
#else
  // even tho this isn't the way we'll port this, the style is fun.
#error port me!
#endif

// gets the filename of a font file guaranteed to be in the system font path.
void Std::getDefaultFont(int bufsize, char *dst) {
  ASSERT(dst != NULL);
  const char *output = default_font;
  STRNCPY(dst, output, bufsize);
}

void Std::setDefaultFont(const char *newdefaultfont) {
  STRNCPY(default_font, newdefaultfont, 255); default_font[255] = 0;
}

int Std::getDefaultFontScale() {
  return default_font_scale;
}

void Std::setDefaultFontScale(int scale) {
  default_font_scale = scale;
}

int Std::createDirectory(const char *dirname) {
#ifdef WIN32
  if(!CreateDirectory(dirname,NULL)) {
    // create all the path
    PathParser pp(dirname);
    int l = pp.getNumStrings();
    for(int i=2;i<=l;i++) {
      String dir;
      for(int j=0;j<i;j++) 
        dir += StringPrintf("%s\\",pp.enumString(j));
      CreateDirectory(dir,NULL);
    }
  }
  return 1;
#else
#error port me!
  //return mkdir( dirname, 0755 );
  // see previous code here ^^^, however, someone needs to make sure mkdir will
  // create all the intermediate dirs too!
#endif
}

int Std::getFileInfos(const char *filename, fileInfoStruct *infos) {
  int retval = 0;
#ifdef WIN32

#if UTF8
  if (Std::encodingSupportedByOS(SvcStrCnv::UTF16)) {
    // CODE FOR NT COMPATIBLE OS'S
    EncodedStr path16enc;
    int retcode = path16enc.convertFromUTF8(SvcStrCnv::UTF16, String(filename));
    WCHAR *path16 = static_cast<WCHAR *>(path16enc.getEncodedBuffer());
    if (path16 != NULL) {
      struct _stati64 statbuf;
      if (_wstati64(path16, &statbuf) == -1) return 0;
      infos->lastWriteTime = statbuf.st_mtime;
      infos->fileSizeLow = (int)(statbuf.st_size);
      infos->fileSizeHigh = (int)(statbuf.st_size >> 32);
      infos->readonly = !(statbuf.st_mode & _S_IWRITE);
    } else if (retcode == SvcStrCnv::ERROR_UNAVAILABLE) {
      struct _stati64 statbuf;
      if (_stati64(filename, &statbuf) == -1) return 0;
      infos->lastWriteTime = statbuf.st_mtime;
      infos->fileSizeLow = (int)(statbuf.st_size);
      infos->fileSizeHigh = (int)(statbuf.st_size >> 32);
      infos->readonly = !(statbuf.st_mode & _S_IWRITE);
    }
  } else {
    // CODE FOR 9x COMPATIBLE OS'S
    EncodedStr pathOSenc;
    int retcode = pathOSenc.convertFromUTF8(SvcStrCnv::OSNATIVE, String(filename));
    char *pathOS = static_cast<char *>(pathOSenc.getEncodedBuffer());
    if (pathOS != NULL) {
      struct _stati64 statbuf;
      if (_stati64(pathOS, &statbuf) == -1) return 0;
      infos->lastWriteTime = statbuf.st_mtime;
      infos->fileSizeLow = (int)(statbuf.st_size);
      infos->fileSizeHigh = (int)(statbuf.st_size >> 32);
      infos->readonly = !(statbuf.st_mode & _S_IWRITE);
    } else if (retcode == SvcStrCnv::ERROR_UNAVAILABLE) {
      struct _stati64 statbuf;
      if (_stati64(filename, &statbuf) == -1) return 0;
      infos->lastWriteTime = statbuf.st_mtime;
      infos->fileSizeLow = (int)(statbuf.st_size);
      infos->fileSizeHigh = (int)(statbuf.st_size >> 32);
      infos->readonly = !(statbuf.st_mode & _S_IWRITE);
    }
  }
#else
  HANDLE h;
  WIN32_FIND_DATA fd;
  if((h=FindFirstFile(filename, &fd))==INVALID_HANDLE_VALUE) return 0;

  infos->fileSizeHigh=fd.nFileSizeHigh;
  infos->fileSizeLow=fd.nFileSizeLow;
  struct _stati64 statbuf;
  if (_stati64(filename, &statbuf) == -1) return 0;
  infos->lastWriteTime = statbuf.st_mtime;
  infos->readonly = fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
  FindClose(h);
#endif
  return 1;
#else
  struct stat st;

  int ret = stat( filename, &st );
  if ( ret != 0 )
    return 0;

  infos->fileSizeHigh = 0;
  infos->fileSizeLow = st.st_size;
  infos->lastWriteTime = st.st_mtime;
  return 1;
#endif
}

void Std::removeDirectory(const char *buf, int recurse) {
#ifdef WIN32
#if UTF8
  int failed = 0;
  if (Std::encodingSupportedByOS(SvcStrCnv::UTF16)) {
    // CODE FOR NT COMPATIBLE OS'S
    EncodedStr buf16enc;
    int retcode = buf16enc.convertFromUTF8(SvcStrCnv::UTF16, String(buf));
    WCHAR *buf16 = static_cast<WCHAR *>(buf16enc.getEncodedBuffer());
    if (retcode == SvcStrCnv::ERROR_UNAVAILABLE) {
      failed = 1;
    } else {
      if (recurse) {
        WIN32_FIND_DATAW fdW;
        char path[WA_MAX_PATH];
        STRCPY(path,buf);
        char *p=path;
        int c = 0;
        while (*p) {
          p++; c++;
        }
        STRCPY(p,"\\*.*");
        // Yah, whatever.  Okay, here we have path.  Convert it.
        EncodedStr path16encstr;
        path16encstr.convertFromUTF8(SvcStrCnv::UTF16, String(path));
        WCHAR *path16enc = static_cast<WCHAR *>(path16encstr.getEncodedBuffer());
        if (!path16enc) return;
        WCHAR path16[WA_MAX_PATH];
        memcpy(path16,path16enc,WSTRLEN(path16enc)+1);
        HANDLE h;
        h = FindFirstFileW(path16,&fdW);
        if (h != INVALID_HANDLE_VALUE) {
          do {
            if (fdW.cFileName[0] != '.' ||
                (fdW.cFileName[1] != '.' && fdW.cFileName[1])) {
              MEMCPY(path16+c+1, fdW.cFileName, (WSTRLEN(fdW.cFileName)+1)*2);
              if (fdW.dwFileAttributes & FILE_ATTRIBUTE_READONLY) 
                SetFileAttributesW(path16,fdW.dwFileAttributes^FILE_ATTRIBUTE_READONLY);
              if (fdW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // now we have to go BACK to UTF8.  christ on a popsicle stick.
                EncodedStr next16enc(SvcStrCnv::UTF16, path16, (WSTRLEN(path16)+1)*2, 0/*no delete*/);
                String path8;
                next16enc.convertToUTF8(path8);
                removeDirectory(path8,recurse);
              }
              else DeleteFileW(path16);
            }
          } while (FindNextFileW(h,&fdW));
          FindClose(h);
        }
        p[0]=0; // fix buffer
      }
      RemoveDirectoryW(buf16);
    }
  } else {
    // CODE FOR 9x COMPATIBLE OS'S
    EncodedStr bufOSenc;
    int retcode = bufOSenc.convertFromUTF8(SvcStrCnv::OSNATIVE, String(buf));
    char *bufOS = static_cast<char *>(bufOSenc.getEncodedBuffer());
    if (retcode == SvcStrCnv::ERROR_UNAVAILABLE) {
      failed = 1;
    } else {
      if (recurse) {
        WIN32_FIND_DATA fd;
        char path[WA_MAX_PATH];
        STRCPY(path,buf);
        char *p=path;
        int c = 0;
        while (*p) {
          p++; c++;
        }
        STRCPY(p,"\\*.*");
        // Yah, whatever.  Okay, here we have path.  Convert it.
        EncodedStr pathOSencstr;
        pathOSencstr.convertFromUTF8(SvcStrCnv::OSNATIVE, String(path));
        char *pathOSenc = static_cast<char *>(pathOSencstr.getEncodedBuffer());
        if (!pathOSenc) return;
        char pathOS[WA_MAX_PATH];
        strcpy(pathOS,pathOSenc);
        HANDLE h;
        h = FindFirstFile(pathOS,&fd);
        if (h != INVALID_HANDLE_VALUE) {
          do {
            if (fd.cFileName[0] != '.' ||
                (fd.cFileName[1] != '.' && fd.cFileName[1])) {
              STRCPY(pathOS+c+1, fd.cFileName);
              if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) 
                SetFileAttributes(pathOS,fd.dwFileAttributes^FILE_ATTRIBUTE_READONLY);
              if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // now we have to go BACK to UTF8.  christ on a popsicle stick.
                EncodedStr nextOSenc(SvcStrCnv::OSNATIVE, pathOS, (STRLEN(pathOS)+1), 0/*no delete*/);
                String path8;
                nextOSenc.convertToUTF8(path8);
                removeDirectory(path8,recurse);
              }
              else DeleteFile(pathOS);
            }
          } while (FindNextFile(h,&fd));
          FindClose(h);
        }
        p[0]=0; // fix buffer
      }
      RemoveDirectory(bufOS);
    } 
  }
  if (failed) {
    if (recurse) {
      WIN32_FIND_DATA fd;
      char path[WA_MAX_PATH];
      STRCPY(path,buf);
      char *p=path;
      while (*p) p++;
      STRCPY(p,"\\*.*");
      HANDLE h;
      h = FindFirstFile(path,&fd);
      if (h != INVALID_HANDLE_VALUE) {
        do {
          if (fd.cFileName[0] != '.' ||
              (fd.cFileName[1] != '.' && fd.cFileName[1])) {
            STRCPY(p+1,fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) 
              SetFileAttributes(path,fd.dwFileAttributes^FILE_ATTRIBUTE_READONLY);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) removeDirectory(path,recurse);
            else DeleteFile(path);
          }
        } while (FindNextFile(h,&fd));
        FindClose(h);
      }
      p[0]=0; // fix buffer
    }
    RemoveDirectory(buf);
  }
#else
  if (recurse) {
    WIN32_FIND_DATA fd;
    char path[WA_MAX_PATH];
    STRCPY(path,buf);
    char *p=path;
    while (*p) p++;
    STRCPY(p,"\\*.*");
    HANDLE h;
    h = FindFirstFile(path,&fd);
    if (h != INVALID_HANDLE_VALUE) {
      do {
        if (fd.cFileName[0] != '.' ||
            (fd.cFileName[1] != '.' && fd.cFileName[1])) {
          STRCPY(p+1,fd.cFileName);
          if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) 
            SetFileAttributes(path,fd.dwFileAttributes^FILE_ATTRIBUTE_READONLY);
          if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) removeDirectory(path,recurse);
          else DeleteFile(path);
        }
      } while (FindNextFile(h,&fd));
      FindClose(h);
    }
    p[0]=0; // fix buffer
  }
  RemoveDirectory(buf);
#endif
#else
  rmdir( buf );
  if ( recurse ) {
    OutputDebugString( StringPrintf( "portme -- Std::removeDirector( %s, recurse )\n", buf ) );
  }
#endif
}

#ifdef WASABI_COMPILE_FILEREADER
int Std::fileExists(const char *filename) {
  return WASABI_API_FILE->fileExists(filename);
}

int Std::isDirectory(const char *filename) {
#ifdef WIN32
  if (WASABI_API_FILE != NULL) {
#pragma CHAT("bas", "rOn", "might want to check for dir via readers here")
    // return WASABI_API_FILE->fileIsDirectory();
  }
  unsigned int r = GetFileAttributes(filename);
  if (r == 0xffffffff) return 0;
  return !!(r & FILE_ATTRIBUTE_DIRECTORY);
#else
#error port me
#endif
}
#endif

void Std::shellExec(const char *cmd, const char *params) {
#ifdef WIN32
  ShellExecute(NULL, NULL, cmd, params, ".", SW_SHOWNORMAL);
#else
  system(StringPrintf("%s %s", cmd, params));
#endif
}

int Std::isLittleEndian() {
  return ((*((int*)"\1  "))&1);  // That one was justin's favourite.  I didn't test it.
}


int Std::encodingSupportedByOS(const FOURCC enc) {
#ifdef WIN32
  int retval = 0;
#if UTF8
  // For windows, this is it.
  switch (enc) {
    case SvcStrCnv::OSNATIVE:
      retval = 1; // duh.
    case SvcStrCnv::UTF16: {
      static int staticval = -1;
      if (staticval == -1) {
        // Initialize your capabilities just once, please
        CreateFileW(L"Hello, world",0,0,NULL,0,0,NULL);
        // Yes, we will fail, but why?
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
          // Not implemented means we're on 98 (or 98 style API's)
          staticval = 0;
        } else {
          staticval = 1;
        }

      }
      retval = staticval;
    }
    break;
    default:
      retval = 0;
    break;
  }
#endif
  return retval;
#else
//PORTME
  return 0;
#endif
}

#endif // nostudio

void MEMFILL32(void *lptr, unsigned long val, unsigned int n) {
  if (n == 0) return;
#ifdef WIN32
__asm {
  mov eax, val
  mov edi, lptr
  mov ecx, n
  rep stosd
};
#elif defined(GCC)
//http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html ;)
asm ("cld\n\t"
     "rep\n\t"
     "stosl"
     : /* no output registers */
     : "c" (n), "a" (val), "D" (lptr)
     : "%ecx", "%edi" );
#else
#error port me!
#endif
}

void MEMFILL(unsigned short *ptr, unsigned short val, unsigned int n) {
  if (n == 0) return;
  unsigned long v = (unsigned long)val | ((unsigned long)val << 16);
  int r = n & 1;
  MEMFILL32(ptr, v, n/2);
  if (r) ptr[n-1] = val;
}

