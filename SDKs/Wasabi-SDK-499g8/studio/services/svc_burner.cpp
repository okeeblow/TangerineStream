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
#include "svc_burner.h"
#include "../api.h"

#define CBCLASS svc_mediaRecorderI
START_DISPATCH;
  CB(ISSESSIONSUPPORTED,isSessionSupported)
  CB(ISMEDIASUPPORTED,isMediaSupported)
  CB(GETNUMDEVICES,getNumDevices)
  CB(ENUMDEVICE,enumDevice)
  VCB(REFRESHDEVICES,refreshDevices)
END_DISPATCH;
#undef CBCLASS

#define CBCLASS MediaRecorder::DeviceI
START_DISPATCH;
  CB(GETDEPENDENCYPTR,getDependencyPtr)
  CB(GETDEVICENAME,getDeviceName)
  CB(GETDEVICETYPE,getDeviceType)
  CB(GETDEVICEDESCRIPTION,getDeviceDescription)
  CB(ENUMDEVICESPEEDS,enumDeviceSpeeds)
  CB(GETMEDIASIZE,getMediaSize)
  CB(GETMEDIAFREE,getMediaFree)
  VCB(CLEARSESSIONS,clearSessions)
  CB(ADDSESSION,addSession)
  CB(GETSESSION,getSession)
  CB(SETRECORDSPEED,setRecordSpeed)
  CB(SETTEST,setTest)
  CB(SETCLOSEDISC,setCloseDisc)
  CB(CANBURNNOW,canBurnNow)
  CB(CANCANCEL,canCancel)
  CB(BEGIN,begin)
  CB(END,end)
  CB(CANCEL,cancel)
  CB(GETSTATUS,getStatus)
  CB(GETPROGRESS,getProgress)
  CB(GETSTATUSTEXT,getStatusText)
  CB(GETLASTERROR,getLastError)
END_DISPATCH;
#undef CBCLASS

#define CBCLASS MediaRecorder::SessionI
START_DISPATCH;
  CB(GETSESSIONTYPE,getSessionType)
  CB(CLOSESESSION,closeSession)
  CB(GETNUMENTRIES,getNumEntries)
  CB(ENUMENTRY,enumEntry)
  CB(GETTOTALBYTES,getTotalBytes)
  CB(GETTOTALTIME,getTotalTime)
END_DISPATCH;
#undef CBCLASS

const char *MediaRecorder::RedbookSession::enumEntry(int n) {
  if( n>=getNumEntries()) return NULL;
  return m_tracks[n]->getValue(); 
}

int MediaRecorder::RedbookSession::getTotalBytes() {
  double length=(double)getTotalTime();
  return (int)(length*(44100*4)/1000); //always 44khz 16bps stereo
}

int MediaRecorder::RedbookSession::getTotalTime() {
  int total=0;
  for(int i=0;i<getNumEntries();i++) {
    int length=0;
    if((length=api->metadb_getLength(m_tracks[i]->getValue()))!=-1) total+=length;
  }
  return total;
}
