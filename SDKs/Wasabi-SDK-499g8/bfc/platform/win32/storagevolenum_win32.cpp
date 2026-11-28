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
#include "storagevolenum_win32.h"

StorageVolumeWin32::StorageVolumeWin32(const char *name) {
  size = -1;
  freespace = -1;
  type = 0;
  mountPath = NULL;
  labelName = NULL;

  volumeName = name;
}

StorageVolumeWin32::~StorageVolumeWin32() {

}

const char *StorageVolumeWin32::getVolumeName() {
  return volumeName;
}

const char *StorageVolumeWin32::getMountPath() {
  if(mountPath == NULL) {
#ifdef WIN32
    mountPath = volumeName;
#endif
  }
  
  return mountPath.getValueSafe();
}

const char *StorageVolumeWin32::getLabel() {
  if(labelName == NULL) {
#ifdef WIN32
    char buffer[4096];
    DWORD maxFilenameLen;
    DWORD serialNum;
    DWORD fsFlags;
    if(GetVolumeInformation(volumeName, buffer, 4096, &serialNum, &maxFilenameLen, &fsFlags, NULL, 0)) {
      buffer[4095] = 0;
      labelName = buffer;
    }
#endif // WIN32
  }

  return labelName.getValueSafe();
}

int StorageVolumeWin32::getType() {
  if(type == 0) {
#ifdef WIN32
    UINT _type = GetDriveType(volumeName);
    if(_type == DRIVE_NO_ROOT_DIR) type |= NOT_VALID;
    if(_type == DRIVE_UNKNOWN) type |= UNKNOWN;
    if(_type == DRIVE_REMOVABLE) type |= REMOVABLE;
    if(_type == DRIVE_FIXED) type |= LOCAL;
    if(_type == DRIVE_REMOTE) type |= NETWORK;
    if(_type == DRIVE_CDROM) type |= CDROM|REMOVABLE;
    if(_type == DRIVE_RAMDISK) type |= RAMDISK;
#endif // WIN32
  }
  return type;
}

int StorageVolumeWin32::isRemovable() {
  return (type & REMOVABLE > 0);
}

int StorageVolumeWin32::isWritable() {

  return 0;
}

__int64 StorageVolumeWin32::getFreeSpace() {
  if(freespace == -1) {
#ifdef WIN32
    FARPROC p = GetProcAddress(GetModuleHandle("kernel32.dll"),"GetDiskFreeSpaceExA");
    if(p) {
      ULARGE_INTEGER _freespace;
      ULARGE_INTEGER _space;
      ULARGE_INTEGER _available;
    
      if(GetDiskFreeSpaceEx(volumeName, &_available, &_space, &_freespace)) {
        size = _space.QuadPart;
        freespace = _freespace.QuadPart;
      }

    } else {
      DWORD sectorsPerCluster;
      DWORD bytesPerSector;
      DWORD freeClusters;
      DWORD totalClusters;
      if(GetDiskFreeSpace(volumeName, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters)) {
        size = totalClusters * sectorsPerCluster * bytesPerSector;
        freespace = freeClusters * sectorsPerCluster * bytesPerSector;
      }
    }
#endif // WIN32
  }

  return freespace;
}

__int64 StorageVolumeWin32::getSize() {
  if(size == -1) {
#ifdef WIN32
    FARPROC p = GetProcAddress( GetModuleHandle("kernel32.dll"),"GetDiskFreeSpaceExA");
    if(p) {
      ULARGE_INTEGER _freespace;
      ULARGE_INTEGER _space;
      ULARGE_INTEGER _available;
    
      if(GetDiskFreeSpaceEx(volumeName, &_available, &_space, &_freespace)) {
        size = _space.QuadPart;
        freespace = _freespace.QuadPart;
      }

    } else {
      DWORD sectorsPerCluster;
      DWORD bytesPerSector;
      DWORD freeClusters;
      DWORD totalClusters;
      if(GetDiskFreeSpace(volumeName, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters)) {
        size = totalClusters * sectorsPerCluster * bytesPerSector;
        freespace = freeClusters * sectorsPerCluster * bytesPerSector;
      }
    }
#endif // WIN32
  }

  return size;
}

StorageVolumeEnumWin32::StorageVolumeEnumWin32() {
  getAllVolumes();
}

StorageVolumeEnumWin32::~StorageVolumeEnumWin32() {
  volumes.freeAll();
}

int StorageVolumeEnumWin32::getNumVolumes() {
  return volumes.getNumItems();
}

StorageVolume *StorageVolumeEnumWin32::enumVolume(int which) {
  StorageVolumeWin32 *sv = NULL;
  String *vol = NULL;
  if((vol = volumes.enumItem(which)) != NULL) {
    sv = new StorageVolumeWin32(vol->getValueSafe());
  }
  return static_cast<StorageVolume *>(sv);
}

void StorageVolumeEnumWin32::getAllVolumes() {
#ifdef WIN32
  char *p;
  char buffer[4096] = {0};

  DWORD len = GetLogicalDriveStrings(4096, &buffer[0]);
  buffer[4095] = 0;

  p = &buffer[0];

  while(len) {
    String *str = new String(p);
    volumes.addItem(str);

    p+=str->len()+1;
    len-=str->len()+1;
  }
#endif // WIN32
  
  return;
}