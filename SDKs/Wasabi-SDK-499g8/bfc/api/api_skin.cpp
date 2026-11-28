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
#include "api_skin.h"

#ifdef CBCLASS
#undef CBCLASS
#endif
#define CBCLASS api_skinI
START_DISPATCH;
  CB(API_SKIN_SKIN_GETCOLORELEMENT, skin_getColorElement);
  CB(API_SKIN_SKIN_GETCOLORELEMENTREF, skin_getColorElementRef);
  CB(API_SKIN_SKIN_GETITERATOR, skin_getIterator);
  VCB(API_SKIN_SKIN_SWITCHSKIN, skin_switchSkin);
  VCB(API_SKIN_SKIN_UNLOADSKIN, skin_unloadSkin);
  CB(API_SKIN_GETSKINNAME, getSkinName);
  CB(API_SKIN_GETSKINPATH, getSkinPath);
  CB(API_SKIN_GETSKINSPATH, getSkinsPath);
  CB(API_SKIN_GETDEFAULTSKINPATH, getDefaultSkinPath);
  CB(API_SKIN_IMGLDR_REQUESTSKINBITMAP, imgldr_requestSkinBitmap);
  VCB(API_SKIN_IMGLDR_RELEASESKINBITMAP, imgldr_releaseSkinBitmap);
  CB(API_SKIN_FILTERSKINCOLOR, filterSkinColor);
  VCB(API_SKIN_REAPPLYSKINFILTERS, reapplySkinFilters);
  CB(API_SKIN_COLORTHEME_GETNUMCOLORSETS, colortheme_getNumColorSets);
  CB(API_SKIN_COLORTHEME_ENUMCOLORSET, colortheme_enumColorSet);
  CB(API_SKIN_COLORTHEME_GETNUMCOLORGROUPS, colortheme_getNumColorGroups);
  CB(API_SKIN_COLORTHEME_ENUMCOLORGROUPNAME, colortheme_enumColorGroupName);
  CB(API_SKIN_COLORTHEME_ENUMCOLORGROUP, colortheme_enumColorGroup);
  CB(API_SKIN_COLORTHEME_GETCOLORGROUP, colortheme_getColorGroup);
  VCB(API_SKIN_COLORTHEME_SETCOLORSET, colortheme_setColorSet);
  CB(API_SKIN_COLORTHEME_GETCOLORSET, colortheme_getColorSet);
  VCB(API_SKIN_COLORTHEME_NEWCOLORSET, colortheme_newColorSet);
  VCB(API_SKIN_COLORTHEME_UPDATECOLORSET, colortheme_updateColorSet);
  VCB(API_SKIN_COLORTHEME_RENAMESET, colortheme_renameColorSet);
  VCB(API_SKIN_COLORTHEME_DELETE, colortheme_deleteColorSet);
  CB(API_SKIN_LOADSKINFILE, loadSkinFile);
  CB(API_SKIN_LOADGROUPDEFDATA, loadGroupDefData);
  VCB(API_SKIN_UNLOADSKINPART, unloadSkinPart);
  CB(API_SKIN_GROUP_CREATE, group_create);
#ifdef WASABI_COMPILE_CONFIG
  CB(API_SKIN_GROUP_CREATE_CFG, group_create_cfg);
#endif // WASABI_COMPILE_CONFIG
#ifdef WASABI_COMPILE_WNDMGR
  CB(API_SKIN_GROUP_CREATE_LAYOUT, group_create_layout);
#endif //WASABI_COMPILE_WNDMGR
  CB(API_SKIN_GROUP_DESTROY, group_destroy);
  CB(API_SKIN_GROUP_EXISTS, group_exists);
  CB(API_SKIN_PARSE, parse);
  CB(API_SKIN_XUI_NEW, xui_new);
  VCB(API_SKIN_XUI_DELETE, xui_delete);
  CB(API_SKIN_CURSOR_REQUEST, cursor_request);
  CB(API_SKIN_GETNUMGROUPS, getNumGroupDefs);
  CB(API_SKIN_ENUMGROUP, enumGroupDef);
  CB(API_SKIN_GROUP_CREATEBYITEM, group_createBySkinItem);
  CB(API_SKIN_GETGROUPANCESTOR, getGroupDefAncestor);
  CB(API_SKIN_GROUPDEF_GETNUMOBJECTS, groupdef_getNumObjects);
  CB(API_SKIN_GROUPDEF_ENUMOBJECT, groupdef_enumObject);
  VCB(API_SKIN_SETLOCKUI, skin_setLockUI);
  CB(API_SKIN_GETLOCKUI, skin_getLockUI);
  CB(API_SKIN_GETVERSION, skin_getVersion);
  CB(API_SKIN_GETBITMAPCOLOR, skin_getBitmapColor);
END_DISPATCH;
