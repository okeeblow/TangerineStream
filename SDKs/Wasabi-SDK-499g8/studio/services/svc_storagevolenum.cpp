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
#include "svc_storagevolenum.h"

//-----------------------------------------------------------------

#define CBCLASS StorageVolumeI
START_DISPATCH;
  CB(GETVOLUMENAME, getVolumeName);
  CB(GETMOUNTPATH, getMountPath);
  CB(GETLABEL, getLabel);
  CB(GETTYPE, getType);
  CB(ISREMOVABLE, isRemovable);
  CB(ISWRITABLE, isWritable);
  CB(GETFREESPACE, getFreeSpace);
  CB(GETSIZE, getSize);
END_DISPATCH;
#undef CBCLASS

//-----------------------------------------------------------------

#define CBCLASS svc_storageVolumeEnumI
START_DISPATCH;
  CB(GETNUMVOLUMES, getNumVolumes);
  CB(ENUMVOLUME, enumVolume);
END_DISPATCH;
#undef CBCLASS

//-----------------------------------------------------------------


