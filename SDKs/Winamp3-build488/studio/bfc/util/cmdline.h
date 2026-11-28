/*

  Nullsoft WASABI Source File License

  Copyright 1999-2001 Nullsoft, Inc.

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


  Brennan Underwood
  brennan@nullsoft.com

*/

#ifndef _CMDLINE_H
#define _CMDLINE_H

#include "pathparse.h"
#include "../string.h"

// utility class for parsing command lines that look like so
// app.exe /blah /gleh /something:param /blah:param1,param2 nakeditem

class CmdLine : public PathParser {
public:
  CmdLine(const char *cmdline) : PathParser(cmdline, " ") { }

  int hasSwitch(const char *switchname) {
    return (getSwitchPos(switchname) >= 0);
  }
  int isSwitch(int pos) {
    const char *p = enumString(pos);
    if (p == NULL) return 0;
    return (*p == SWITCHCHAR);
  }
  int getSwitchPos(const char *switchname) {
    ASSERT(switchname != NULL);
    String str;
    str.printf("%c%s", SWITCHCHAR, switchname);
    for (int i = 0; i < getNumStrings(); i++) {
      PathParser pp(enumString(i), "=");
      if (STRCASEEQL(pp.enumString(0), str)) return i;
    }
    return -1;
  }

  // app.exe /switchname:param1,param2,etc
  String getSwitchParam(const char *switchname, int parampos=0) {
    int pos = getSwitchPos(switchname);
    if (pos < 0) return String((const char *)NULL);
    PathParser pp(enumString(pos), "=");
    const char *s = pp.enumString(1);
    if (s == NULL) return String((const char *)NULL);
    PathParser pp2(s, ",");
    return String(pp2.enumString(parampos));
  }

protected:
  virtual void preProcess(String &str) {
    char *nstr = str.getNonConstVal();
    for (int in=0; *nstr; nstr++) {
      if (*nstr == '"') in ^= 1;
      else if (in && *nstr == ' ') *nstr = 1;
    }
  }
  virtual void postProcess(char *str) {
    for (; *str; str++) {
      if (*str == 1) *str = ' ';
      else if (*str == '"') {
        STRCPY(str, str+1);
        str--;
      }
    }
  }
};

#endif
