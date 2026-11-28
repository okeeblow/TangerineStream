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
#ifndef __LINUX_H_WASABI
#define __LINUX_H_WASABI

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <math.h>

#include <sys/shm.h>

#ifndef __USE_GNU
#define __USE_GNU
#include <pthread.h>
#undef __USE_GNU
#else
#include <pthread.h>
#endif

// Fucking Region and Font namespace conflicts
#define _XRegion _XRegion
#define Region HRGN
#define Font HFONT
#define Cursor _Cursor
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#undef _XRegion
#undef Region
#undef Font
#undef Cursor

#ifdef __INCLUDE_FREETYPE
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>

//typedef FT_Face HFONT;
#endif

#ifdef __INCLUDE_GTK
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

// I know about XRectangle, but it's easier to recreate this than it is
// to merge it with the rest of the code
typedef struct { int left, top, right, bottom; } RECT; 

typedef Window HWND;
typedef unsigned long COLORREF;
typedef struct {
  char rgbBlue;
  char rgbGreen;
  char rgbRed;
  char filler;
} RGBQUAD;
#define RGB( r, g, b ) ((((r)&0xFF)<<16)|(((g)&0xFF)<<8)|((b)&0xFF))
#define min( a, b ) ((a>b)?b:a)
#define max( a, b ) ((a>b)?a:b)
#define CLR_NONE    0

#ifdef __cplusplus
#ifndef XMD_H
typedef int BOOL; // It's int not bool because of some inheritance conflicts
#endif
#define TRUE  true
#define FALSE false
#else
#ifndef XMD_H
typedef int BOOL;
#endif
#define TRUE  1
#define FALSE 0
#endif
typedef BOOL BOOLEAN;
typedef struct { int x, y; } POINT;
typedef void *HPEN;

// The HDC knows what it's drawing on
typedef struct hdc_typ {
  GC gc;
  Drawable d;
  HRGN clip;
} *HDC;

// Pixmaps don't have associated width and height, at least not that I know of
typedef struct {
  Pixmap p;
  int bmWidth;
  int bmHeight;
  XShmSegmentInfo *shmseginfo;

} BITMAP, HBITMAP;

typedef Pixmap HICON;
typedef int LRESULT;
typedef int LPARAM;
typedef int WPARAM;
typedef int RPARAM;
typedef unsigned int TCHAR;
typedef long long __int64;
typedef long long LARGE_INTEGER;
typedef unsigned long long ULARGE_INTEGER;

#define LOWORD(a) ((a)&0xffff)
#define HIWORD(a) (((a)>>16)&0xffff)

#define MAX_PATH 8192

#define COMEXP
#define EXTC extern "C"
#define __cdecl
#define CALLBACK
#define WINAPI
#define HRESULT void*
#define WINUSERAPI
#define APIENTRY
#define __declspec(a)
typedef char * LPSTR;
typedef unsigned long DWORD;
typedef short int WORD;
#ifndef XMD_H
typedef unsigned char BYTE;
#endif
typedef void* LPVOID;
typedef struct {
  long cx, cy;
} SIZE;
typedef long LONG;

// Fix this for canvas
typedef void * PAINTSTRUCT;

typedef void* THREAD_RET;


// Fix this with editwnd!
typedef void * WNDPROC;
typedef void * DRAWITEMSTRUCT;

#define VK_MENU    (XK_Alt_L | (XK_Alt_R << 16))
#define VK_MBUTTON (XK_Meta_L | (XK_Meta_R << 16))
#define VK_SHIFT   (XK_Shift_L | (XK_Shift_R << 16))
#define MK_RBUTTON ((XK_VoidSymbol << 16) | 1)
#define MK_LBUTTON ((XK_VoidSymbol << 16) | 2)
#define VK_CONTROL (XK_Control_L | (XK_Control_R << 16))
#define VK_DELETE  (XK_Delete | (XK_KP_Delete << 16))
#define VK_RETURN  (XK_Return)
#define VK_ESCAPE  (XK_Escape)
#define VK_DOWN    (XK_Down)
#define VK_UP      (XK_Up)
#define VK_LEFT    (XK_Left)
#define VK_RIGHT   (XK_Right)
#define VK_HOME    (XK_Home)
#define VK_END     (XK_End)
#define VK_PRIOR   (XK_Prior)
#define VK_NEXT    (XK_Next)
#define VK_BACK    (XK_BackSpace)
#define VK_F1      (XK_F1)
#define VK_SPACE   (XK_space)

#define INVALID_HANDLE_VALUE NULL

// Come back here later
typedef struct {
  pthread_mutex_t mutex;
  pthread_t id;
  int count;
} CRITICAL_SECTION;
typedef pthread_t HANDLE;

typedef void (*TIMERPROC)(HWND, UINT, UINT, DWORD);
typedef char OLECHAR;

//#define NO_MMX
//CUT? #define NULLREGION    0

#define MAIN_MINX 0
#define MAIN_MINY 0

typedef struct {
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
} MSG;

#ifdef __cplusplus

#include <new>

#define NULLREGION    1
#define SIMPLEREGION  2
#define COMPLEXREGION 3

enum {
  WM_CREATE, // MULL
  WM_CLOSE, // NULL
  WM_PAINT, // NULL
  WM_NCPAINT, // NULL
  WM_SYNCPAINT, // NULL
  WM_SETCURSOR, // NULL
  WM_TIMER, // timerid
  WM_SETFOCUS, // NULL
  WM_KILLFOCUS, // NULL
  WM_LBUTTONDOWN, // xPos | yPos << 16
  WM_RBUTTONDOWN, // "
  WM_MOUSEMOVE, // "
  WM_LBUTTONUP, // "
  WM_RBUTTONUP, // "
  WM_CONTEXTMENU, // "
  WM_ERASEBKGND, // NULL
  WM_MOUSEWHEEL, // a << 16 | t (l=a/120)
  WM_CHAR, // char
  WM_KEYDOWN, // keypress
  WM_KEYUP, // "
  WM_SYSKEYDOWN, // look at OnSysKeyDown
  WM_SYSKEYUP, // "
  WM_SYSCOMMAND, // Hunh?
  WM_MOUSEACTIVATE, // Hunh?
  WM_ACTIVATEAPP, // Hunh?
  WM_ACTIVATE, // WA_ACTIVE || WA_CLICKACTIVE
  WM_NCACTIVATE, // NULL
  WM_WINDOWPOSCHANGED, // NULL, WINDOWPOS *
  WM_DROPFILES, // HDROP
  WM_CAPTURECHANGED, // NULL
  WM_COMMAND, // Commands?..
  WM_SETTINGCHANGE,
  WM_QUIT,
  WM_DESTROY,
  WM_USER = 1000, // wParam, lParam -> make sure this is last
};

enum {
  WA_ACTIVE, WA_CLICKACTIVE,
};

enum {
  SC_CLOSE,
};

enum {
  DT_LEFT,
  DT_CENTER,
  DT_RIGHT,
  DT_VCENTER,
  DT_WORDBREAK,
  DT_SINGLELINE,
  DT_NOPREFIX,
  DT_PATH_ELLIPSIS,
  DT_END_ELLIPSIS,
  DT_MODIFYSTRING,
  DT_CALCRECT,

};

#define ALL_EVENTS       ExposureMask | StructureNotifyMask | \
		         KeyPressMask | KeyReleaseMask | \
		         ButtonPressMask | ButtonReleaseMask | \
                         PointerMotionMask | FocusChangeMask | \
                         EnterWindowMask | LeaveWindowMask
                         

#define NO_INPUT_EVENTS  ExposureMask | StructureNotifyMask | \
                         PointerMotionMask | FocusChangeMask | \
                         EnterWindowMask | LeaveWindowMask 

class Linux {
 private:
  static Display *display;
  static XContext context;

 public:
  static Display *getDisplay();
  static int getScreenNum();
  static Window RootWin();
  static Visual *DefaultVis();

  static int convertEvent( MSG *, XEvent * );
  static void initContextData( HWND h );
  static void nukeContextData( HWND h );
  static XContext getContext();
  static void setCursor( HWND h, int cursor );
};

#endif

typedef void* HINSTANCE; // Useless, just a placeholder
typedef void* WIN32_FIND_DATA;
typedef void* WIN32_FIND_DATAW;
typedef void* BLENDFUNCTION;
typedef void* ATOM;
typedef void* HGLOBAL;
typedef void* HKEY;
typedef char* LPTSTR;
typedef char* LPCTSTR;
typedef DWORD* LPDWORD;

int GetMessage( MSG *, HWND, UINT, UINT );
void TranslateMessage( MSG * );
int DispatchMessage( MSG * );
int SendMessage( HWND, UINT, WPARAM, LPARAM );
void PostMessage( HWND, UINT, WPARAM, LPARAM );
void PostQuitMessage( int );
void OutputDebugString( const char *s );
int MulDiv( int m1, int m2, int d );
DWORD GetTickCount();
void Sleep( int ms );
int IntersectRect(RECT *, const RECT *, const RECT *);
int SetTimer( HWND, int id, int ms, TIMERPROC );
void KillTimer( HWND, int id );
void ExitProcess( int ret );

enum contextdata { 
  GWL_HINSTANCE = 0, 
  GWL_USERDATA, 
  GWL_INVALIDREGION,
  GWL_RECT_LEFT,
  GWL_RECT_TOP,
  GWL_RECT_RIGHT,
  GWL_RECT_BOTTOM,
  GWL_HWND,
  GWL_PARENT,

  GWL_ENUM_SIZE 
};
void MoveWindowRect( HWND, int, int );
void SetWindowRect( HWND, RECT * );
int GetWindowRect( HWND, RECT * );
void SetWindowLong( HWND, enum contextdata, LONG );
LONG GetWindowLong( HWND, enum contextdata );
int GetUpdateRect( HWND, RECT *, BOOL );
void GetUpdateRgn( HWND, HRGN, BOOL );
void InvalidateRgn( HWND, HRGN, BOOL );
void InvalidateRect( HWND, const RECT *, BOOL );
void ValidateRgn( HWND, HRGN );
void ValidateRect( HWND, const RECT * );
int SubtractRect( RECT *out, RECT *in1, RECT *in2 );
int EqualRect( RECT *a, RECT *b );

HWND GetActiveWindow();
HWND WindowFromPoint( POINT p );

void CopyFile( const char *f1, const char *f2, BOOL b );

#endif

