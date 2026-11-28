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
#ifndef _WINFO_H
#define _WINFO_H

#include <bfc/string.h>
#include <core/mediainfo.h>

class WInfo {
public:
  WInfo();
  ~WInfo();

  int Open(const char *url); // returns 0 on success.

  const char *GetLastPlaystring();

  void GetTitle(char *title, int maxlen);
  void GetInfo(char *info, int maxlen);
  int GetLength(void);     /* get length of track in ms. -1 indicates infinite/unknown */

  int GetMetaData(char *name, char *data, int data_len); // returns size of data
  int SetMetaData(char *name, char *data, int data_len); // returns 0 on success, nonzero on error

private:
  MediaInfoI mi;
  String last_playstring;
};

#endif
