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
#ifndef _INIFILE_H
#define _INIFILE_H

#include "../string.h"

class IniFile {
public:
  IniFile(const char *_filename);

  void setString(const char *section, const char *tagname, const char *val);
  char *getString(const char *section, const char *tagname, char *buf, int buflen, const char *default_val = ""); // returns buf
  String getString(const char *section, const char *tagname, const char *default_val="");

  void setInt(const char *section, const char *tagname, int val);
  int getInt(const char *section, const char *tagname, int default_val = 0);

  int getBool(const char *section, const char *tagname, int default_val = 0);
  void setBool(const char *section, const char *tagname, int val);

  GUID getGuid(const char *section, const char *tagname, GUID default_val = INVALID_GUID);
  void setGuid(const char *section, const char *tagname, const GUID &val);

private:
  String filename;
};

#endif
