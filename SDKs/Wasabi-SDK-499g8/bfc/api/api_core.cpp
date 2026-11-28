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
#include "api_core.h"

#ifdef CBCLASS
#undef CBCLASS
#endif
#define CBCLASS api_coreI
START_DISPATCH;
  CB(API_CORE_GETSUPPORTEDEXTENSIONS, core_getSupportedExtensions);
  CB(API_CORE_GETEXTSUPPORTEDEXTENSIONS, core_getExtSupportedExtensions);
  CB(API_CORE_CREATE, core_create);
  CB(API_CORE_FREE, core_free);
  CB(API_CORE_SETNEXTFILE, core_setNextFile);
  CB(API_CORE_GETSTATUS, core_getStatus);
  CB(API_CORE_GETCURRENT, core_getCurrent);
  CB(API_CORE_GETCURPLAYBACKNUMBER, core_getCurPlaybackNumber);
  CB(API_CORE_GETPOSITION, core_getPosition);
  CB(API_CORE_GETWRITEPOSITION, core_getWritePosition);
  CB(API_CORE_SETPOSITION, core_setPosition);
  CB(API_CORE_GETLENGTH, core_getLength);
  CB(API_CORE_GETPLUGINDATA, core_getPluginData);
  CB(API_CORE_GETVOLUME, core_getVolume);
  VCB(API_CORE_SETVOLUME, core_setVolume);
  CB(API_CORE_GETPAN, core_getPan);
  VCB(API_CORE_SETPAN, core_setPan);
  VCB(API_CORE_ADDCALLBACK, core_addCallback);
  VCB(API_CORE_DELCALLBACK, core_delCallback);
  CB(API_CORE_GETVISDATA, core_getVisData);
  CB(API_CORE_GETLEFTVUMETER, core_getLeftVuMeter);
  CB(API_CORE_GETRIGHTVUMETER, core_getRightVuMeter);
  CB(API_CORE_REGISTERSEQUENCER, core_registerSequencer);
  CB(API_CORE_DEREGISTERSEQUENCER, core_deregisterSequencer);
  VCB(API_CORE_USERBUTTON, core_userButton);
  CB(API_CORE_GETEQSTATUS, core_getEqStatus);
  VCB(API_CORE_SETEQSTATUS, core_setEqStatus);
  CB(API_CORE_GETEQPREAMP, core_getEqPreamp);
  VCB(API_CORE_SETEQPREAMP, core_setEqPreamp);
  CB(API_CORE_GETEQBAND, core_getEqBand);
  VCB(API_CORE_SETEQBAND, core_setEqBand);
  CB(API_CORE_GETEQAUTO, core_getEqAuto);
  VCB(API_CORE_SETEQAUTO, core_setEqAuto);
  VCB(API_CORE_SETCUSTOMMSG, core_setCustomMsg);
  VCB(API_CORE_REGISTEREXTENSION, core_registerExtension);
  CB(API_CORE_GETEXTENSIONFAMILY, core_getExtensionFamily);
  VCB(API_CORE_UNREGISTEREXTENSION, core_unregisterExtension);
  CB(API_CORE_GETTITLE, core_getTitle);
END_DISPATCH;
