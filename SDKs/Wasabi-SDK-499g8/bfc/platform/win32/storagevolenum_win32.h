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
#ifndef __STORAGEVOLENUM_WIN32_H
#define __STORAGEVOLENUM_WIN32_H

#include "../../../studio/services/svc_storagevolenum.h"

#include "../../../bfc/string.h"
#include "../../../bfc/ptrlist.h"

class StorageVolumeWin32 : public StorageVolumeI {
public:
  StorageVolumeWin32(const char *name);
  virtual ~StorageVolumeWin32();

  virtual const char *getVolumeName();  //Device name, under linux "/dev/hda1", windows "C:"
  virtual const char *getMountPath();   //Path to mounted volume, under windows "C:\", under unix "/home"
  virtual const char *getLabel();       //Get the Drive label, "MYDRIVE" 
  
  virtual int getType();                //Get the type bitfield.
  
  virtual int isRemovable();            //This is a removable / removable media drive.
  virtual int isWritable();             //Possible to write to device.
  
  virtual __int64 getFreeSpace();         //Freespace in bytes.
  virtual __int64 getSize();              //Total Drive Size in bytes.
  
private:

  String volumeName;
  String mountPath;
  String labelName;

  int    type;

  __int64 size;
  __int64 freespace;
};

class StorageVolumeEnumWin32 : public svc_storageVolumeEnumI {
public:
  StorageVolumeEnumWin32();  //Only enumerate drives of type 'type'.
  virtual ~StorageVolumeEnumWin32();

  static const char *getServiceName() { return "Win32 Storage Volume Enumerator";  }

  virtual int getNumVolumes();      //Get the number of Storage Volumes.
                            
                            //Enum a Storage Volume.
  virtual StorageVolume *enumVolume(int which); 

private:
  void getAllVolumes();
  
  PtrList<String> volumes;
  
};

#endif //__STORAGEVOLENUM_WIN32_H