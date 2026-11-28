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
#ifndef __SVC_STORAGEVOLENUM_H
#define __SVC_STORAGEVOLENUM_H

#include "../../bfc/dispatch.h"
#include "services.h"

enum StorageVolumeTypes {
  NOT_VALID   = 0,      //Not a valid Volume.
  UNKNOWN     = 1,      //Unknown Drive Type.
  LOCAL       = 1<<1,   //Local (Fixed) Hard Drive.
  REMOVABLE   = 1<<2,   //Removable Drive (Floppy, LS-120, Zip, USB FlashCard Reader, etc.)
  NETWORK     = 1<<3,   //Network Drive (SMB, NFS, etc.)
  CDROM       = 1<<4,   //CD / DVD ROM, WRITER, Re-WRITER, etc. 
  RAMDISK     = 1<<5,   //RAM Drive.
};

//-----------------------------------------------------------------

class NOVTABLE StorageVolume : public Dispatchable {
public:
  const char *getVolumeName() { return _call(GETVOLUMENAME, (const char *) NULL); }
  const char *getMountPath() { return _call(GETMOUNTPATH, (const char *) NULL); }
  const char *getLabel() { return _call(GETLABEL, (const char *) NULL);  }

  int getType() { return _call(GETTYPE, 0);  }

  int isRemovable() { return _call(ISREMOVABLE, -1);  }
  int isWritable() { return _call(ISWRITABLE, -1);  }

  __int64 getFreeSpace() { return _call(GETFREESPACE, -1); }
  __int64 getSize() { return _call(GETSIZE, -1); }

  enum {
    GETVOLUMENAME = 10,
    GETMOUNTPATH  = 20,
    GETLABEL      = 30,
    GETTYPE       = 40,
    ISREMOVABLE   = 50,
    ISWRITABLE    = 60,
    GETFREESPACE  = 70,
    GETSIZE       = 80
  };
};

//-----------------------------------------------------------------

class StorageVolumeI : public StorageVolume {
public:
  virtual const char *getVolumeName()=0;
  virtual const char *getMountPath()=0;
  virtual const char *getLabel()=0;

  virtual int getType()=0;

  virtual int isRemovable()=0;
  virtual int isWritable()=0;

  virtual __int64 getFreeSpace()=0;
  virtual __int64 getSize()=0;

protected:
  RECVS_DISPATCH;
  
};

//-----------------------------------------------------------------

class svc_storageVolumeEnum : public Dispatchable {
public:
  static FOURCC getServiceType() { return WaSvc::STORAGEVOLENUM; }

  int getNumVolumes() { return _call(GETNUMVOLUMES, (int) 0); }
  StorageVolume *enumVolume(int which) { return _call(ENUMVOLUME, (StorageVolume *)NULL);  }

  enum {
    GETNUMVOLUMES = 10,
    ENUMVOLUME    = 20,
  };

};

//-----------------------------------------------------------------

class svc_storageVolumeEnumI : public svc_storageVolumeEnum {
public:
  virtual int getNumVolumes()=0;      //Get the number of Storage Volumes.
                                      //Enum a Storage Volume.
  virtual StorageVolume *enumVolume(int which)=0;

protected:
  RECVS_DISPATCH;

};

#endif //__SVC_STORAGEVOLENUM_H