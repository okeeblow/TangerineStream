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
#ifndef _STD_MATH_H
#define _STD_MATH_H

// FG> doesn't work for me without this include (error C2039: 'sin' : is not a member of '`global namespace'')
#include <math.h>

#ifdef __cplusplus
static inline double SIN(double a) { return ::sin(a); }
static inline double COS(double a) { return ::cos(a); }
static inline double SQRT(double a) { return ::sqrt(a); }

static inline unsigned long BSWAP(unsigned long input) {
#ifdef _MSC_VER
  _asm {
    mov eax, input
    bswap eax
    mov input, eax
  };
  return input;
#else
#  ifdef GCC
  __asm__ volatile (
		    "\tmov %0, %%eax\n"
		    "\tbswap %%eax\n"
		    : "=m" (input)
		    : 
		    : "%eax" );
#  else
  unsigned char ret[4];
  unsigned char *s = reinterpret_cast<unsigned char *>(&input);
  ret[0] = s[3];
  ret[1] = s[2];
  ret[2] = s[1];
  ret[3] = s[0];
  return *reinterpret_cast<unsigned long *>(ret);
#  endif
#endif
}
#else
#define SIN(a) sin(a)
#define COS(a) sin(a)
#define SQRT(a) sqrt(a)
unsigned long COMEXP BSWAP_C(unsigned long input);
#define BSWAP BSWAP_C
#endif

#ifdef __cplusplus
// neat trick from C++ book, p. 161
template<class T> inline T MAX(T a, T b) { return a > b ? a : b; }
template<class T> inline T MIN(T a, T b) { return a > b ? b : a; }
template<class T> inline T MINMAX(T a, T minval, T maxval) {
  return (a < minval) ? minval : ( (a > maxval) ? maxval : a );
}

// and a couple of my own neat tricks :) BU
template<class T> inline T ABS(T a) { return a < 0 ? -a : a; }
template<class T> inline T SQR(T a) { return a * a; }
template<class T> inline int CMP3(T a, T b) {
  if (a < b) return -1;
  if (a == b) return 0;
  return 1;
}

static inline RGB24 RGBTOBGR(RGB24 col) {
  return ((col & 0xFF00FF00) | ((col & 0xFF0000) >> 16) | ((col & 0xFF) << 16));
}
static inline RGB24 BGRTORGB(RGB24 col) { return RGBTOBGR(col); }
static inline ARGB32 BGRATOARGB(ARGB32 col) { return RGBTOBGR(col); }

void COMEXP premultiplyARGB32(ARGB32 *words, int nwords=1);

class COMEXP StdMath {
public:
  static int mulDiv(int m1, int m2, int d);
};

#else // not __cplusplus
void COMEXP premultiplyARGB32(ARGB32 *words, int nwords);
#endif

#endif
