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

#include "std_math.h"

unsigned long COMEXP BSWAP_C(unsigned long input) {
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

void premultiplyARGB32(ARGB32 *words, int nwords) {
  for (; nwords > 0; nwords--, words++) {
    unsigned char *pixel = (unsigned char *)words;
    unsigned int alpha = pixel[3];
    if (alpha == 255) continue;
    pixel[0] = (pixel[0] * alpha) >> 8;	// blue
    pixel[1] = (pixel[1] * alpha) >> 8;	// green
    pixel[2] = (pixel[2] * alpha) >> 8;	// red
  }
}

int StdMath::mulDiv(int m1, int m2, int d) {
#ifdef WIN32
  return MulDiv(m1, m2, d);
#elif defined(LINUX)
  __asm__ volatile (
		    "mov %0, %%eax\n"
		    "mov %1, %%ebx\n"
		    "mov %2, %%ecx\n"
		    "mul %%ebx\n"
		    "div %%ecx\n"
		    : : "m" (m1), "m" (m2), "m" (d) 
		    : "%eax", "%ebx", "%ecx", "%edx" );
#else
#error port me!
#endif
}
