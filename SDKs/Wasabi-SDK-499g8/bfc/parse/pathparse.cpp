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

#include "pathparse.h"

PathParser::PathParser(const char *_str, const char *sep, int uniquestrs) :
  processed(FALSE), str(_str ? _str : ""), separators(sep), uniques(uniquestrs)
{
  ASSERT(sep != NULL);
}

int PathParser::getNumStrings() {
  process();
  return strings.getNumItems();
}

char *PathParser::enumString(int i) {
  process();
  return strings[i];
}

char *PathParser::enumStringSafe(int i, char *def_val) {
  char *ret = enumString(i);
  if (ret == NULL) ret = def_val;
  return ret;
}

void PathParser::process() {
  if (processed) return;
  processed = 1;
  preProcess(str);
  char *nonconst = str.getNonConstVal();
  
  char *pt = strtok(nonconst, separators);
  if (pt == NULL) return;
  postProcess(pt);
  strings.addItem(pt);
  for (;;) {
    char *pt = strtok(NULL, separators);
    if (pt == NULL) break;
    postProcess(pt);
    if (uniques) {
      int exists = 0;
      foreach(strings)
        if (STRCASEEQL(strings.getfor(), pt)) {
          exists=1;
          break;
        }
      endfor;
      if (exists) continue;
    }
    strings.addItem(pt);
  }
}

