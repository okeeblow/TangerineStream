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

#include "inifile.h"
#include "../../common/nsguid.h"

IniFile::IniFile(const char *_filename) : filename(_filename) { }

#ifdef WIN32
void IniFile::setString(const char *section, const char *tagname, const char *val) {
  WritePrivateProfileString(section, tagname, val, filename);
}

char *IniFile::getString(const char *section, const char *tagname, char *buf, int buflen, const char *default_val) {
  GetPrivateProfileString(section, tagname, default_val, buf, buflen, filename);
  return buf;
}
#else
  #error port me!
#endif

String IniFile::getString(const char *section, const char *tagname, const char *default_val) {
  char buf[WA_MAX_PATH]="";
  getString(section, tagname, buf, WA_MAX_PATH-1, default_val);
  return String(buf);
}

void IniFile::setInt(const char *section, const char *tagname, int val) {
  setString(section, tagname, StringPrintf(val));
}

int IniFile::getInt(const char *section, const char *tagname, int default_val) {
  char buf[MAX_PATH];
  getString(section, tagname, buf, sizeof(buf), StringPrintf(default_val));
  return ATOI(buf);
}

int IniFile::getBool(const char *section, const char *tagname, int default_val) {
  char buf[MAX_PATH];
  getString(section, tagname, buf, sizeof(buf), default_val ? "true" : "false");
  if (STRCASEEQL(buf, "true")) return 1;
  return 0;
}
  
void IniFile::setBool(const char *section, const char *tagname, int val) {
  setString(section, tagname, val ? "true" : "false");
}

GUID IniFile::getGuid(const char *section, const char *tagname, GUID default_val) {
  char buf[MAX_PATH];
  getString(section, tagname, buf, sizeof(buf), StringPrintf(default_val));
  return nsGUID::fromChar(buf);
}

void IniFile::setGuid(const char *section, const char *tagname, const GUID &val) {
  setString(section, tagname, StringPrintf(val));
}
