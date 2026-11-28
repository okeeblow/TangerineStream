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

#include "winfo.h"

#include <studio/services/svc_mediaconverter.h>
#include <studio/services/servicei.h>
#include <studio/api.h>

#include <bfc/svc_enum.h>

WInfo::WInfo() {
  mi.setLength(-1);
}

WInfo::~WInfo() { }

int WInfo::Open(const char *filename)
{
  last_playstring = filename;
  if(!STRNINCMP(filename,"file:")) filename+=5;

  svc_fileReader *reader=NULL;
  // find the correct reader
  waServiceFactory *readerf=NULL;
  int i;
  for(i=0;readerf=api->service_enumService(WaSvc::FILEREADER,i);i++) {
    svc_fileReader *fr=castService<svc_fileReader>(readerf, FALSE);
    if(!fr->canPrefetch()) {
      readerf->releaseInterface(fr);
      continue;
    }
    if(fr->open(filename)) {
      reader=fr;
      break;
    } else {
	  readerf->releaseInterface(fr); fr = NULL;
	}
  }

  if (reader == NULL) return 1;

  waServiceFactory *s;
  for(i=0;s=api->service_enumService(WaSvc::MEDIACONVERTER,i);i++) {
    svc_mediaConverter *mc=castService<svc_mediaConverter>(s, FALSE);
    if(mc->canConvertFrom(reader, filename, NULL)) {
      mi.setFilename(filename);
      mi.setReader(reader);
      mc->getInfos(&mi);
      s->releaseInterface(mc);
      readerf->releaseInterface(reader);
      return 0;
    } else s->releaseInterface(mc);
  }
  if (readerf && reader) readerf->releaseInterface(reader);
  return 1;
}

const char *WInfo::GetLastPlaystring() {
  return last_playstring;
}

void WInfo::GetTitle(char *title, int maxlen)
{
  STRNCPY(title,mi.getTitle(),maxlen);
  title[maxlen-1]=0;
}
void WInfo::GetInfo(char *info, int maxlen)
{
  STRNCPY(info,mi.getInfo(),maxlen);
  info[maxlen-1]=0;
}

int  WInfo::GetLength(void)
{
  return mi.getLength();
}


int  WInfo::GetMetaData(char *name, char *data, int data_len) // returns size of data
{
  STRNCPY(data,mi.getData(name),data_len);
  data[data_len-1]=0;
  if(!data[0]) return 0;
  return STRLEN(data);
} 
