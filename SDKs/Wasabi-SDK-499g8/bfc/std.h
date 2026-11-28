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
#ifndef _STD_H
#define _STD_H

#include <bfc/platform/platform.h>
#include <common/common.h>
#include <studio/assert.h>

#ifdef __cplusplus
#include <wasabicfg.h>
#endif

//#define WANT_UTF8_WARNINGS

// to use this, do #pragma WARNING("your message") <- note the ABSENCE of ';'
#ifndef WARNING
  #define WARNING_TOSTR(text) #text
  #define WARNING_TOSTR1(text) WARNING_TOSTR(text)
  #define WARNING(text) message(__FILE__ "(" WARNING_TOSTR1(__LINE__) ") : " text)
#endif
// and for this one, do #pragma COMPILATION_CHAT("your_nick", "nick_you're_talking_to", "your message") 
#define CHAT(from, to, text) message(__FILE__ "(" WARNING_TOSTR1(__LINE__) ") : <" from "> " to ": " text)
#define SELF(from, text) message(__FILE__ "(" WARNING_TOSTR1(__LINE__) ") : * " from"/#wasabi " text)

#ifndef WASABIDLLEXPORT
#error go define WASABIDLLEXPORT in your platform .h
#endif

#ifndef NOVTABLE
#error go define NOVTABLE in your platform .h
#endif

#include "std_mem.h"
#include "std_math.h"
#include "std_string.h"
#include "std_file.h"

#define WASABI_DEFAULT_FONTNAME "Arial"

static __inline int TOUPPER(int c) { return toupper(c); }
static __inline int TOLOWER(int c) { return tolower(c); }
static __inline int ISDIGIT(int c) { return isdigit(c); }
static __inline int ISALPHA(int c) { return isalpha(c); }
static __inline int ISSPACE(int c) { return isspace(c); }
static __inline int ISPUNCT(int c) { return ispunct(c); }
static __inline int ATOI(const char *str) { if (!str) return 0; return atoi(str); }
static __inline double ATOF(const char *str) { if (!str) return 0.0; return atof(str); }
static __inline int STRTOL(const char *str, char **stopstr, int base) {
  return strtol(str, stopstr, base);
}
// hex to int
static __inline int XTOI(const char *str) {
  char *dummy;
  return STRTOL(str, &dummy, 16);
}

COMEXP char *DO_STRDUP(const char *ptr EXTRA_INFO);
COMEXP int STRLEN(const char *str);
COMEXP int STRCMP(const char *str1, const char *str2);
COMEXP int STRICMP(const char *str1, const char *str2);
COMEXP int STRNCMP(const char *str1, const char *str2, int len);
COMEXP int STRNNCMP(const char *str1, const char *str2); // uses len of str2
COMEXP int STRNICMP(const char *str1, const char *str2, int len);
COMEXP int STRNINCMP(const char *str1, const char *str2); // uses len of str2
COMEXP int STREQL(const char *str1, const char *str2);
COMEXP int STRCASEEQL(const char *str1, const char *str2);
COMEXP char *STRCASESTR(const char *str1, const char *str2);
COMEXP char *STRSTR(const char *str1, const char *str2);
COMEXP int STRCASERPL(char *s, const char *w, const char *r, int maxchar);
COMEXP void STRCPY(char *dest, const char *src);
COMEXP void STRNCPY(char *dest, const char *src, int maxchar);
COMEXP char *STRCHR(const char *str, int c);
COMEXP char *STRRCHR(const char *str, int c);
COMEXP void STRCAT(char *dest, const char *append);
COMEXP unsigned long STRTOUL(const char *str, char **p, int radix);

#ifdef __cplusplus
COMEXP int STRCMPSAFE(const char *str1, const char *str2, const char *defval1="", const char *defval2="");
COMEXP int STRICMPSAFE(const char *str1, const char *str2, const char *defval1="", const char *defval2="");
COMEXP int STREQLSAFE(const char *str1, const char *str2, const char *defval1="", const char *defval2="");
COMEXP int STRCASEEQLSAFE(const char *str1, const char *str2, const char *defval1="", const char *defval2="");
#endif

COMEXP int PATHEQL(const char *str1, const char *str2);
COMEXP void STRTOUPPER(char *str);
COMEXP void STRTOLOWER(char *str);
COMEXP int STRISNUM(const char *p);

// Bare-bones utf16 support.  Just transform to utf8 as soon as possible, 
// However, these are to make your life somewhat easier.
COMEXP int WSTRLEN(const wchar_t *str16);
COMEXP wchar_t *DO_WSTRDUP(const char *str8 EXTRA_INFO);
COMEXP void WSTRCAT(wchar_t *dest, const wchar_t *append);
COMEXP void WSTRCAT7(wchar_t *dest, const char *append);
COMEXP int WSTRCMP(const wchar_t *a, const wchar_t *b);
COMEXP int WSTRCMP7(const wchar_t *a, const char *b);

#ifdef WIN32
#define SPRINTF wsprintf
#else
#define SPRINTF sprintf
#endif

#define SSCANF sscanf

#define WA_MAX_PATH (8*1024)

// seconds since 1970
typedef unsigned int stdtimeval;
// milliseconds since...??
typedef double stdtimevalms;

typedef unsigned long THREADID;

#ifdef __cplusplus

#include "string.h"

class COMEXP Std {
public:
  Std();

  static void getMousePos(POINT *p);
  static void getMousePos(int *x, int *y);
  static void getMousePos(long *x, long *y);
  static void setMousePos(POINT *p);
  static void setMousePos(int x, int y);

  static int keyDown(int code);
  
  static int pointInRect(RECT *r, POINT &p);
  static void setRect(RECT *r, int left, int top, int right, int bottom);
  static RECT makeRect(int left, int top, int right, int bottom);
  static POINT makePoint(int x, int y);
  static void offsetRect(RECT *r, int x, int y);
  static int rectEqual(const RECT &a, const RECT &b);
  static int rectEqual(const RECT *a, const RECT *b);
  static int rectIntersect(const RECT *a, const RECT *b, RECT *intersection=NULL);

  static void srandom(unsigned int key=0);//if key==0, uses time()
  static int random(int max = RAND_MAX+1);// from 0 to max-1 (RAND_MAX = 0x7fff)
  static unsigned int random32(unsigned int max = 0xffffffff);	// full 32-bits of randomness

  // time functions
  static void usleep(int ms);
  // get time in seconds since 1970
  static stdtimeval getTimeStamp();
  // get time in seconds as double, but not since any specific time
  // useful for relative timestamps only
  static stdtimevalms getTimeStampMS();
  // get milliseconds since system started. usefull for relative only
  static DWORD getTickCount();

  static void tolowerString(char *str);
  static void ensureVisible(RECT *r);
  static int getScreenWidth();
  static int getScreenHeight();
// THREADS/PROCESSES/CPUs
  static int getNumCPUs();
  // a unique # returned by the OS
  static THREADID getCurrentThreadId();
  // attempts to adjust thread priority compared to main thread
  // normal range is from -2 .. +2, and -32767 for idle, 32767 for time critical
  // use the constants from bfc/thread.h
  static void setThreadPriority(int delta, HANDLE thread_handle = NULL);

  static String getLastErrorString(int force_errno=-1);

  static int messageBox(const char *txt, const char *title, int flags);

  static int getDoubleClickDelay();
  static int getDoubleClickX();
  static int getDoubleClickY();

  // returns how many lines to scroll for wheelie mice (from the OS)
  static int osparam_getScrollLines();
  static int osparam_getSmoothScroll();

  static int dirChar();	// \ for win32, / for all else
#define DIRCHAR (Std::dirChar())
  static const char *dirCharStr(); // "\\" for win32, "/" for all else
#define DIRCHARSTR (Std::dirCharStr())
  static int isDirChar(int thechar, int allow_multiple_platforms = TRUE);
  static const char *matchAllFiles(); // "*.*" on win32, "*" on else
#define MATCHALLFILES (Std::matchAllFiles())
  static const char *dotDir();	// usually "."
#define DOTDIR (Std::dotDir())
  static const char *dotDotDir();	// usually ".."
#define DOTDOTDIR (Std::dotDotDir())

  static int isRootPath(const char *path); // "c:\" or "\" on win32, "/" on linux

  static int switchChar(); 	// '/' on win32, '-' for all else
#define SWITCHCHAR (Std::switchChar())

  static void getViewport(RECT *r, POINT *p, int full = 0);
  static void getViewport(RECT *r, RECT *sr, int full = 0);
  static void getViewport(RECT *r, HWND wnd, int full = 0);
  static void getViewport(RECT *r, POINT *p, RECT *sr, HWND wnd, int full = 0);
  static int enumViewports(int monitor_n, RECT *r, int full = 0);

  // returns the address of the last occurence of any of the characters of toscan in str string
  static const char *scanstr_back(const char *str, const char *toscan, const char *defval);

  // retrieves extension of a given filename
  static const char *extension(const char *fn);

  // retrieves filename from a given path+filename
  static const char *filename(const char *fn);

  static int getCurDir(char *str, int maxlen);
  static int setCurDir(const char *str);

  // regexp match functions

  // A match means the entire string TEXT is used up in matching.
  // In the pattern string:
  //      `*' matches any sequence of characters (zero or more)
  //      `?' matches any character
  //      [SET] matches any character in the specified set,
  //      [!SET] or [^SET] matches any character not in the specified set.

  // A set is composed of characters or ranges; a range looks like
  // character hyphen character (as in 0-9 or A-Z).  [0-9a-zA-Z_] is the
  // minimal set of characters allowed in the [..] pattern construct.
  // Other characters are allowed (ie. 8 bit characters) if your system
  // will support them.

  // To suppress the special syntactic significance of any of `[]*?!^-\',
  // and match the character exactly, precede it with a `\'.

  enum {
    MATCH_VALID=1,  /* valid match */
    MATCH_END,      /* premature end of pattern string */
    MATCH_ABORT,    /* premature end of text string */
    MATCH_RANGE,    /* match failure on [..] construct */
    MATCH_LITERAL,  /* match failure on literal match */
    MATCH_PATTERN,  /* bad pattern */
  };

  enum {
    PATTERN_VALID=0,  /* valid pattern */
    PATTERN_ESC=-1,   /* literal escape at end of pattern */
    PATTERN_RANGE=-2, /* malformed range in [..] construct */
    PATTERN_CLOSE=-3, /* no end bracket in [..] construct */
    PATTERN_EMPTY=-4, /* [..] contstruct is empty */
  };

  // return TRUE if PATTERN has any special wildcard characters
  static BOOL isMatchPattern(const char *p);

  // return TRUE if PATTERN has is a well formed regular expression
  static BOOL isValidMatchPattern(const char *p, int *error_type);

  // return MATCH_VALID if pattern matches, or an errorcode otherwise
  static int matche(register const char *p, register const char *t);
  static int matche_after_star(register const char *p, register const char *t);

  // return TRUE if pattern matches, FALSE otherwise.
  static BOOL match(const char *p, const char *t);

  // gets the system temporary path.
  static void getTempPath(int bufsize, char *dst);

  // gets the system font path.
  static void getFontPath(int bufsize, char *dst);

  // gets the filename of a font file guaranteed to be in the system font path.
  static void getDefaultFont(int bufsize, char *dst);
  
  // sets a new default font, ie "blah.ttf"
  static void setDefaultFont(const char *newdefaultfont);

  // gets the default font scale
  static int getDefaultFontScale();
  
  // sets the new default font scale
  static void setDefaultFontScale(int scale);

  // creates a directory. returns 0 on error, nonzero on success
  static int createDirectory(const char *dirname);

  // gets informations about a given filename. returns 0 if file not found
  typedef struct {
  unsigned int fileSizeHigh;
  unsigned int fileSizeLow;
  stdtimeval lastWriteTime;
  int readonly;
  } fileInfoStruct;

  static int getFileInfos(const char *filename, fileInfoStruct *infos);

  // removes a directory
  static void removeDirectory(const char *buf, int recurse);

  // checks if a file exists
  static int fileExists(const char *filename);

  // checks if file is a directory
  static int isDirectory(const char *filename);

  static void shellExec(const char *cmd, const char *params=NULL);

  static int isLittleEndian();

  // unicode & internationalization support.
  static int encodingSupportedByOS(const FOURCC enc);
};

#endif

#endif
