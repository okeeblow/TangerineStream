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

#include "nchoosek.h"

// this DOESN'T WORK yet, I'm still finishing it BU

NChooseK::NChooseK(int n, int k) {
  ASSERT(n >= 0);
  ASSERT(k >= 0);
  ASSERT(k <= n);

  set.setSize(n);

  char *mem = set.getMemory();

#if 0
  int np = 0;
  for (int i = 0; i < k; i++) {
    int pt = Std::random32(k-i);
    for (int j = 0; j < pt; j++) {
      for(;;) {
        np++;
        if (np >= n) np = 0;
        if (mem[np]) continue;
        mem[np] = TRUE; // ok, set it
        break;
      }
    }
  }
#else
  mem[Std::random32(n)] = TRUE;	//hehe
#endif
}

int NChooseK::isSet(int i) {
  if (i < 0 || i >= set.getSize()) return FALSE; //???
  return set.getMemory()[i];
}
