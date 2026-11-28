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
#ifndef PP_CRT_H
#define PP_CRT_H

#ifndef _DEBUG
// release optimizations
// /Og (global optimizations), /Os (favor small code), /Oy (no frame pointers)
//#pragma optimize("gsy",on)
#pragma comment(linker,"/RELEASE")
// set the 512-byte alignment (only in VC6+)
#if _MSC_VER >= 1200
#pragma comment(linker,"/opt:nowin98")
#endif
#endif

#ifndef PP_NO_INTR
#define INTR_HACK
#endif

#define PP_CRT_INSIDE

#ifdef  __cplusplus
extern "C" {
#endif

#pragma check_stack( off )

#define STRICT
#include <windows.h>
//shut up common useless warnings
#pragma warning(disable:4005)
#pragma warning(disable:4244)
#pragma warning(disable:4800)
#pragma warning(disable:4305)


/*
#ifndef PP_USE_CONSOLE
#define PP_NO_CONSOLE
#endif

#ifndef PP_USE_ARG
#define PP_NO_ARG
#endif

#ifndef PP_USE_MEM
#define PP_NO_MEM
#endif
*/

#if !defined(PP_NO_ARG) || !defined(PP_NO_MEM) || !defined(PP_NO_CONSOLE)
void _cdecl crt_init();
#else
#define _cdecl crt_init()
#endif

void _cdecl crt_quit();//only for leakhunt shiz

#ifndef PP_NO_ARG
#define PP_MAX_ARG 16
extern unsigned int argc;
extern char* argv[PP_MAX_ARG];
#endif	//PP_NO_ARG

#ifndef PP_NO_CONSOLE

#define _INC_STDIO

void putchar(int c);
int getchar();
int kbhit();
void print(char* s);
void print_i(int i);
void print_ui(unsigned int i);
#define puteol() {putchar(13);putchar(10);}
extern HANDLE hStdOut;
extern HANDLE hStdIn;
#endif	//PP_NO_CONSOLE

#ifndef PP_NO_MEM

#define _INC_MALLOC

extern HANDLE hHeap;


#define calloc(x,y) z_malloc((x)*(y))

void * _cdecl z_malloc(int);

void * _cdecl _alloca(int);
#define alloca _alloca

#endif	//PP_NO_MEM

void _fastcall _zeromem(UINT,void*);
void _cdecl r_memcpy(void* p1,void* p2,UINT s);	//cheap memmove replacement

double _cdecl floor(double);

#ifdef INTR_HACK

#define _INC_MATH
//hack; MSVC 'intrinsic' funcs
//#pragma optimize("i",on) MSVC5 doesn't get it
		
		double	__cdecl ldexp(double,int);
		double  __cdecl exp(double);
		double  __cdecl atan(double);
		double  __cdecl atan2(double,double);
		long	__cdecl labs(long);
		double  __cdecl fabs(double);
		double  __cdecl sin(double);
		double  __cdecl cos(double);
        double  __cdecl tan(double);
        double  __cdecl sqrt(double);
		double  __cdecl log(double);
#define log10(X) (log(X)/log(10))
        int     __cdecl memcmp(const void *, const void *, size_t);
        void *  __cdecl memcpy(void *, const void *, size_t);
        void *  __cdecl memset(void *, int, size_t);
		char *  __cdecl strcpy(char *, const char *);
		char *  __cdecl strcat(char *, const char *);
		int     __cdecl strcmp(const char *, const char *);
		UINT    __cdecl strlen(const char *);


#define ZeroMemory(x,y) _zeromem(y,x);
//#define strcat(s1,s2) strcpy(s1+strlen(s1),s2)

#else
		void _inline strcat(char* s1,const char* s2) {strcpy(s1+strlen(s1),s2);}

char* _fastcall f_strcpy(char*,const char*);
#define strcpy f_strcpy
double _stdcall f_atan2(double y,double x);
#define atan2 f_atan2

#endif

void _cdecl srand(UINT);
int _cdecl rand();

double _stdcall f_atof(const char* s);
#define atof f_atof

void _stdcall f_gcvt(double val,char* out);
#define gcvt(A,B,C) f_gcvt(A,C)

int f_atoi(char* s);
UINT atoui(char* s);
void f_itoa(int i,char* s);
void uitoa(unsigned int i,char* s);

#define atoi f_atoi
#define itoa f_itoa
#define _itoa(X,Y,Z) __itoa(X,Y)

//text file access used by some of my projects
UINT read_line(HANDLE f,char* buf,UINT s);
char* scan_fn(char* s,char* buf);
char* scan_int(char* s,UINT *r);
char* skip_space(char* s);


double _stdcall f_acos(double x);
#define acos f_acos

double _stdcall f_pow(double a,double b);
#define pow f_pow

//hacks...
#include <mbstring.h>
#define _mbsinc CharNext

#include <direct.h>
#include <io.h>
#include <stdio.h>

#define _mkdir(x) (!CreateDirectory(x,0))
#define mkdir _mkdir
#define sprintf wsprintf
#define _unlink(x) (!DeleteFile(x))
#define unlink _unlink


#define wcstombs(a,b,c) WideCharToMultiByte(CP_ACP,0,b,-1,a,c,0,0)
#define mbstowcs(a,b,c)	MultiByteToWideChar(CP_ACP,0,b,-1,a,c)

DWORDLONG _fastcall q_UInt32x32To64(DWORD u1,DWORD u2);
#define UInt32x32To64 q_UInt32x32To64
//int _fastcall q_MulDiv(int n1,int n2,int d);
//#define MulDiv q_MulDiv
int _stdcall div64(__int64 i,int i1);


//setjmp crap
typedef UINT jmp_buf[6];

void __cdecl f_longjmp(void* b, int v);
int __cdecl f_setjmp(void* b);

#define _INC_SETJMP

#define setjmp f_setjmp
#define longjmp f_longjmp


//filehacks

#ifdef NEED_FILEHACKS
#define _INC_STDIO
//not all of these are implemented...
//typedef struct {int blah;} FILE;
FILE * __cdecl fopen(const char *, const char *);
int __cdecl fputc(int, FILE *);
int __cdecl _fputchar(int);
int __cdecl fputs(const char *, FILE *);
size_t __cdecl fread(void *, size_t, size_t, FILE *);
FILE * __cdecl freopen(const char *, const char *, FILE *);
//int __cdecl fsetpos(FILE *, const fpos_t *);
int __cdecl fseek(FILE *, long, int);
long __cdecl ftell(FILE *);
size_t __cdecl fwrite(const void *, size_t, size_t, FILE *);
int __cdecl fclose(FILE *);
#endif

#ifdef  __cplusplus
}
#endif

#define stricmp _stricmp

#endif //PP_CRT_H