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
#ifndef __SKINAPI_H
#define __SKINAPI_H

#include "../../bfc/api/api_skin.h"
#include "../../bfc/string.h"
#include "../../studio/widgets.h"
#include "../../studio/xmlparams.h"

class SkinApi : public api_skinI {
  public:

     SkinApi();
     virtual ~SkinApi();
     virtual void preShutdown();

     virtual COLORREF skin_getColorElement(char *type, const char **color_group = NULL);
     virtual COLORREF *skin_getColorElementRef(char *type, const char **color_group = NULL);
     virtual int *skin_getIterator();
     virtual void skin_switchSkin(const char *skin_name, const char *skin_path);
     virtual void skin_unloadSkin();
     virtual const char *getSkinName();
     virtual const char *getSkinPath();
     virtual const char *getSkinsPath();
     virtual const char *getDefaultSkinPath();
     virtual ARGB32 *imgldr_requestSkinBitmap(const char *file, int *has_alpha, int *x, int *y, int *subw, int *subh, int *w, int *h, int cached);
     virtual void imgldr_releaseSkinBitmap(ARGB32 *bmpbits);
     virtual COLORREF filterSkinColor(COLORREF color, const char *elementid, const char *groupname);
     virtual void reapplySkinFilters();
     virtual int colortheme_getNumColorSets();
     virtual const char *colortheme_enumColorSet(int n);
     virtual int colortheme_getNumColorGroups(const char *colorset);
     virtual const char *colortheme_enumColorGroupName(const char *colorset, int n);
     virtual ColorThemeGroup *colortheme_enumColorGroup(int colorset, int n);
     virtual ColorThemeGroup *colortheme_getColorGroup(const char *colorset, const char *group);
     virtual void colortheme_setColorSet(const char *colorset);
     virtual const char *colortheme_getColorSet();
     virtual void colortheme_newColorSet(const char *set);
     virtual void colortheme_updateColorSet(const char *set);
     virtual void colortheme_renameColorSet(const char *set, const char *newname);
     virtual void colortheme_deleteColorSet(const char *set);
     virtual int loadSkinFile(const char *xmlfile);
     virtual int loadGroupDefData(const char *groupdef, SkinItem **lastgroupitem);
     virtual void unloadSkinPart(int skinpartid);
     virtual RootWnd *group_create(const char *groupid, int scripts_enabled=1);
     virtual int group_exists(const char *groupid);
#ifdef WASABI_COMPILE_CONFIG
     virtual RootWnd *group_create_cfg(const char *groupid, CfgItem *cfgitem, const char *attributename, int scripts_enabled=1);
#endif // WASABI_COMPILE_CONFIG
#ifdef WASABI_COMPILE_WNDMGR
     virtual RootWnd *group_create_layout(const char *groupid, int scripts_enabled=1);
#endif //WASABI_COMPILE_WNDMGR
     virtual int group_destroy(RootWnd *group);
     virtual int parse(const char *str, const char *how);
     virtual GuiObject *xui_new(const char *classname);
     virtual void xui_delete(GuiObject *o);
     virtual OSCURSOR cursor_request(const char *id);

     virtual int getNumGroupDefs();
     virtual SkinItem *enumGroupDef(int n);
     virtual RootWnd *group_createBySkinItem(SkinItem *item, int scripts_enabled=1);
     virtual SkinItem *getGroupDefAncestor(SkinItem *item);
     virtual int groupdef_getNumObjects(SkinItem *item);
     virtual SkinItem *groupdef_enumObject(SkinItem *groupitem, int n);
     virtual void skin_setLockUI(int l) { lockui = l; }
     virtual int skin_getLockUI() { return lockui; }
     virtual double skin_getVersion();
     virtual COLORREF skin_getBitmapColor(const char *id);

     Widgets *widgets;

  private:
    
     String skinspath;
     int lockui;
};

#endif
