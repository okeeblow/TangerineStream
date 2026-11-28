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
#ifndef _INFO_H
#define _INFO_H

#include <core/mediainfo.h>
#include <bfc/string.h>

class Info
{
public:
  Info();
  virtual ~Info();
  virtual int  Open(const char *url); // returns 0 on success.
  virtual void GetTitle(char *title, int maxlen);
  virtual void GetInfo(char *info, int maxlen);
  virtual int  GetLength(void);     /* get length of track in ms. -1 indicates infinite/unknown */

  virtual int  GetMetaData(char *name, char *data, int data_len); // returns size of data
  int  SetMetaData(const char *name, const char *data); // returns 0 on success, nonzero on error

  const char *enumName(int offset) { return mi.enumDataName(offset); }
  const char *enumValue(int offset) { return mi.enumDataValue(offset); }
  int enumAttributes(int offset, int *param1=NULL, int *param2=NULL) { return mi.enumDataAttributes(offset,param1,param2); }
  
  virtual void setInfo(const char *info, const char *value) { mi.setData(info,value); }
  virtual int commitChanges();
  
  const char *getXmlGroup() { return xmlgroup; }
  
private:
  MediaInfoI mi;
  String xmlgroup;
};

#endif
