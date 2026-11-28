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
#include "info.h"

#include <studio/services/svc_mediaconverter.h>
#include <studio/services/servicei.h>
#include <studio/api.h>
#include <bfc/svc_enum.h>

Info::Info() {
  mi.setLength(-1);
  mi.setDataInt("infoedit",1);
}

Info::~Info() { }

int Info::Open(const char *filename) {
  if(!STRNINCMP(filename,"file:")) filename+=5;

  svc_fileReader *reader=NULL;
  // find the correct reader
  if(STRNINCMP(filename,"http://")) {
    waServiceFactory *s;
    for(int i=0;s=api->service_enumService(WaSvc::FILEREADER,i);i++) {
      svc_fileReader *fr=castService<svc_fileReader>(s);
      if(fr->open(filename)) {
        reader=fr;
        break;
      } else api->service_release(fr);
    }
  }

  waServiceFactory *s;
  for(int i=0;s=api->service_enumService(WaSvc::MEDIACONVERTER,i);i++) {
    svc_mediaConverter *mc=castService<svc_mediaConverter>(s);
    if(mc->canConvertFrom(reader, filename, NULL)) {
      mi.setFilename(filename);
      mi.setReader(reader);
      mc->getInfos(&mi);
      xmlgroup=mc->getInfosXmlGroup();
      api->service_release(mc);
      api->service_release(reader);
      return 0;
    } else api->service_release(mc);
  }
  api->service_release(reader);
  return 1;
}

void Info::GetTitle(char *title, int maxlen) {
  STRNCPY(title,mi.getTitle(),maxlen);
  title[maxlen-1]=0;
}
void Info::GetInfo(char *info, int maxlen) {
  STRNCPY(info,mi.getInfo(),maxlen);
  info[maxlen-1]=0;
}

int  Info::GetLength(void) {
  return mi.getLength();
}

// returns size of data
int  Info::GetMetaData(char *name, char *data, int data_len) {
  STRNCPY(data,mi.getData(name),data_len);
  data[data_len-1]=0;
  if(!data[0]) return 0;
  return STRLEN(data);
} 

int Info::commitChanges() {
  const char *filename=mi.getFilename();

  svc_fileReader *reader=NULL;
  // find the correct reader
  if(STRNINCMP(filename,"http://")) {
    waServiceFactory *s;
    for(int i=0;s=api->service_enumService(WaSvc::FILEREADER,i);i++) {
      svc_fileReader *fr=castService<svc_fileReader>(s);
      if(fr->open(filename,SvcFileReader::READ|SvcFileReader::PLUS)) {
        reader=fr;
        break;
      } else api->service_release(fr);
    }
  }

  waServiceFactory *s;
  for(int i=0;s=api->service_enumService(WaSvc::MEDIACONVERTER,i);i++) {
    svc_mediaConverter *mc=castService<svc_mediaConverter>(s);
    if(mc->canConvertFrom(reader, filename, NULL)) {
      mi.setReader(reader);
      int ret=mc->setInfos(&mi);
      api->service_release(mc);
      api->service_release(reader);
      return ret;
    } else api->service_release(mc);
  }
  api->service_release(reader);
  return 0;
}

int Info::SetMetaData(const char *name, const char *data) {
  int param1=0,param2=0;
  int attr=mi.getAttribute(name,&param1,&param2);
  mi.setData(name,data,attr,param1,param2);
  return 0;
}
