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
#include "api_wndmgr.h"

#ifdef CBCLASS
#undef CBCLASS
#endif
#define CBCLASS wndmgr_apiI
START_DISPATCH;
  VCB(WNDMGR_API_WNDTRACKADD, wndTrackAdd);
  VCB(WNDMGR_API_WNDTRACKREMOVE, wndTrackRemove);
  CB(WNDMGR_API_WNDTRACKDOCK, wndTrackDock);
  CB(WNDMGR_API_WNDTRACKDOCK2, wndTrackDock2);
  VCB(WNDMGR_API_WNDTRACKSTARTCOOPERATIVE, wndTrackStartCooperative);
  VCB(WNDMGR_API_WNDTRACKENDCOOPERATIVE, wndTrackEndCooperative);
  CB(WNDMGR_API_WNDTRACKWASCOOPERATIVE, wndTrackWasCooperative);
  VCB(WNDMGR_API_WNDTRACKINVALIDATEALL, wndTrackInvalidateAll);
  CB(WNDMGR_API_SKINWND_TOGGLEBYGUID, skinwnd_toggleByGuid);
  CB(WNDMGR_API_SKINWND_TOGGLEBYGROUPID, skinwnd_toggleByGroupId);
  CB(WNDMGR_API_SKINWND_CREATEBYGUID, skinwnd_createByGuid);
  CB(WNDMGR_API_SKINWND_CREATEBYGROUPID, skinwnd_createByGroupId);
  VCB(WNDMGR_API_SKINWND_DESTROY, skinwnd_destroy);
  CB(WNDMGR_API_SKINWND_GETNUMBYGUID, skinwnd_getNumByGuid);
  CB(WNDMGR_API_SKINWND_ENUMBYGUID, skinwnd_enumByGuid);
  CB(WNDMGR_API_SKINWND_GETNUMBYGROUPID, skinwnd_getNumByGroupId);
  CB(WNDMGR_API_SKINWND_ENUMBYGROUPID, skinwnd_enumByGroupId);
  VCB(WNDMGR_API_SKINWND_ATTACHTOSKIN, skinwnd_attachToSkin);
  CB(WNDMGR_API_SKIN_GETCONTAINER, skin_getContainer);
  CB(WNDMGR_API_SKIN_GETLAYOUT, skin_getLayout);
  VCB(WNDMGR_API_WNDHOLDER_REGISTER, wndholder_register);
  VCB(WNDMGR_API_WNDHOLDER_UNREGISTER, wndholder_unregister);
  CB(WNDMGR_API_MESSAGEBOX, messageBox);
  CB(WNDMGR_API_GETMODALWND, getModalWnd);
  VCB(WNDMGR_API_PUSHMODALWND, pushModalWnd);
  VCB(WNDMGR_API_POPMODALWND, popModalWnd);
  VCB(WNDMGR_API_DRAWANIMATEDRECTS, drawAnimatedRects);
  CB(WNDMGR_API_AUTOPOPUP_REGISTERGUID, autopopup_registerGuid);
  CB(WNDMGR_API_AUTOPOPUP_REGISTERGROUPID, autopopup_registerGroupId);
  VCB(WNDMGR_API_AUTOPOPUP_UNREGISTER, autopopup_unregister);
  CB(WNDMGR_API_AUTOPOPUP_GETNUMGUIDS, autopopup_getNumGuids);
  CB(WNDMGR_API_AUTOPOPUP_ENUMGUID, autopopup_enumGuid);
  CB(WNDMGR_API_AUTOPOPUP_GETNUMGROUPS, autopopup_getNumGroups);
  CB(WNDMGR_API_AUTOPOPUP_ENUMGROUPS, autopopup_enumGroup);
  CB(WNDMGR_API_AUTOPOPUP_ENUMGUIDDESC, autopopup_enumGuidDescription);
  CB(WNDMGR_API_AUTOPOPUP_ENUMGROUPDESC, autopopup_enumGroupDescription);
  CB(WNDMGR_API_VARMGR_TRANSLATE, varmgr_translate);
  CB(WNDMGR_API_NEWDYNAMICCONTAINER, newDynamicContainer);
END_DISPATCH;
