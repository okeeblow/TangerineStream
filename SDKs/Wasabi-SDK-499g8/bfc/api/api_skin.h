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
#ifndef __API_SKIN_H
#define __API_SKIN_H

#include <wasabicfg.h>

#ifndef WASABI_COMPILE_SKIN 
#error "This module requires the skin api" 
#endif 

#include "../dispatch.h"

class RootWnd;
class GuiObject;
class ColorThemeGroup;
class XmlReaderParams;
class SkinItem;

#ifdef WASABI_COMPILE_CONFIG
class CfgItem;
#endif //WASABI_COMPILE_CONFIG

#ifdef WASABI_COMPILE_WNDMGR
class CfgItem;
#endif //WASABI_COMPILE_WNDMGR

#ifndef OSCURSOR
#define OSCURSOR HICON
#endif

class api_skin : public Dispatchable {
  public:
    COLORREF skin_getColorElement(char *type, const char **color_group = NULL);
    COLORREF *skin_getColorElementRef(char *type, const char **color_group = NULL);
    int *skin_getIterator();
    void skin_switchSkin(const char *skin_name, const char *skin_path=NULL);
    void skin_unloadSkin();
    const char *getSkinName();
    const char *getSkinPath();
    const char *getSkinsPath();
    const char *getDefaultSkinPath();
    ARGB32 *imgldr_requestSkinBitmap(const char *file, int *has_alpha, int *x, int *y, int *subw, int *subh, int *w, int *h, int cached);
    void imgldr_releaseSkinBitmap(ARGB32 *bmpbits);
    COLORREF filterSkinColor(COLORREF color, const char *elementid, const char *groupname);
    void reapplySkinFilters();
    int colortheme_getNumColorSets();
    const char *colortheme_enumColorSet(int n);
    int colortheme_getNumColorGroups(const char *colorset);
    const char *colortheme_enumColorGroupName(const char *colorset, int n);
    ColorThemeGroup *colortheme_enumColorGroup(int colorset, int n);
    ColorThemeGroup *colortheme_getColorGroup(const char *colorset, const char *colorgroup);
    void colortheme_setColorSet(const char *colorset);
    const char *colortheme_getColorSet();
    void colortheme_newColorSet(const char *set);
    void colortheme_updateColorSet(const char *set);
    void colortheme_renameColorSet(const char *set, const char *newname);
    void colortheme_deleteColorSet(const char *set);
    int loadSkinFile(const char *xmlfile);
    int loadGroupDefData(const char *groupdef, SkinItem **lastgroupitem);
    void unloadSkinPart(int skinpartid);
    RootWnd *group_create(const char *groupid, int scripts_enabled=1);
    int group_exists(const char *groupid);
#ifdef WASABI_COMPILE_CONFIG
    RootWnd *group_create_cfg(const char *groupid, CfgItem *cfgitem, const char *attributename, int scripts_enabled=1);
#endif // WASABI_COMPILE_CONFIG
#ifdef WASABI_COMPILE_WNDMGR
    RootWnd *group_create_layout(const char *groupid, int scripts_enabled=1);
#endif //WASABI_COMPILE_WNDMGR
    int group_destroy(RootWnd *group);
    int parse(const char *str, const char *how);
    GuiObject *xui_new(const char *classname);
    void xui_delete(GuiObject *o);
    OSCURSOR cursor_request(const char *id);
    int getNumGroupDefs();
    SkinItem *enumGroupDef(int n);
    RootWnd *group_createBySkinItem(SkinItem *groupitem, int scripts_enabled=1);
    SkinItem *getGroupDefAncestor(SkinItem *groupitem);
    int groupdef_getNumObjects(SkinItem *groupitem);
    SkinItem *groupdef_enumObject(SkinItem *item, int n);
    void skin_setLockUI(int l);
    int skin_getLockUI();
    double skin_getVersion(); 
    COLORREF skin_getBitmapColor(const char *bitmapid);

  enum {
    API_SKIN_SKIN_GETCOLORELEMENT = 0,
    API_SKIN_SKIN_GETCOLORELEMENTREF = 10,
    API_SKIN_SKIN_GETITERATOR = 20,
    API_SKIN_SKIN_SWITCHSKIN = 30,
    API_SKIN_SKIN_UNLOADSKIN = 35,
    API_SKIN_GETSKINNAME = 40,
    API_SKIN_GETSKINPATH = 50,
    API_SKIN_GETSKINSPATH = 60,
    API_SKIN_GETDEFAULTSKINPATH = 70,
    API_SKIN_IMGLDR_REQUESTSKINBITMAP = 80,
    API_SKIN_IMGLDR_RELEASESKINBITMAP = 90,
    API_SKIN_FILTERSKINCOLOR = 100,
    API_SKIN_REAPPLYSKINFILTERS = 110,
    API_SKIN_COLORTHEME_GETNUMCOLORSETS = 120,
    API_SKIN_COLORTHEME_ENUMCOLORSET = 130,
    API_SKIN_COLORTHEME_GETNUMCOLORGROUPS = 140,
    API_SKIN_COLORTHEME_ENUMCOLORGROUPNAME = 150,
    API_SKIN_COLORTHEME_ENUMCOLORGROUP = 160,
    API_SKIN_COLORTHEME_GETCOLORGROUP = 165,
    API_SKIN_COLORTHEME_SETCOLORSET = 170,
    API_SKIN_COLORTHEME_GETCOLORSET = 180,
    API_SKIN_COLORTHEME_NEWCOLORSET = 190,
    API_SKIN_COLORTHEME_RENAMESET = 191,
    API_SKIN_COLORTHEME_DELETE = 192,
    API_SKIN_COLORTHEME_UPDATECOLORSET = 193,
    API_SKIN_LOADSKINFILE = 200,
    API_SKIN_UNLOADSKINPART = 210,
    API_SKIN_GROUP_CREATE = 220,
    API_SKIN_GROUP_EXISTS = 225,
#ifdef WASABI_COMPILE_CONFIG
    API_SKIN_GROUP_CREATE_CFG = 230,
#endif // WASABI_COMPILE_CONFIG
#ifdef WASABI_COMPILE_WNDMGR
    API_SKIN_GROUP_CREATE_LAYOUT = 240,
#endif //WASABI_COMPILE_WNDMGR
    API_SKIN_GROUP_DESTROY = 250,
    API_SKIN_PARSE = 260,
    API_SKIN_XUI_NEW = 270,
    API_SKIN_XUI_DELETE = 280,
    API_SKIN_CURSOR_REQUEST = 290,
    API_SKIN_GETNUMGROUPS = 300,
    API_SKIN_ENUMGROUP = 310,
    API_SKIN_GROUP_CREATEBYITEM = 320,
    API_SKIN_GETGROUPANCESTOR = 330,
    API_SKIN_GROUPDEF_GETNUMOBJECTS = 340,
    API_SKIN_GROUPDEF_ENUMOBJECT = 350,
    API_SKIN_LOADGROUPDEFDATA = 360,
    API_SKIN_SETLOCKUI = 370,
    API_SKIN_GETLOCKUI = 380,
    API_SKIN_GETVERSION = 390,
    API_SKIN_GETBITMAPCOLOR = 400,
  };
};

inline COLORREF api_skin::skin_getColorElement(char *type, const char **color_group) {
  return _call(API_SKIN_SKIN_GETCOLORELEMENT, (COLORREF)NULL, type, color_group);
}

inline COLORREF *api_skin::skin_getColorElementRef(char *type, const char **color_group) {
  return _call(API_SKIN_SKIN_GETCOLORELEMENTREF, (COLORREF *)NULL, type, color_group);
}

inline int *api_skin::skin_getIterator() {
  return _call(API_SKIN_SKIN_GETITERATOR, (int *)0);
}

inline void api_skin::skin_switchSkin(const char *skin_name, const char *skin_path) {
  _voidcall(API_SKIN_SKIN_SWITCHSKIN, skin_name, skin_path);
}

inline void api_skin::skin_unloadSkin() {
  _voidcall(API_SKIN_SKIN_UNLOADSKIN);
}

inline const char *api_skin::getSkinName() {
  return _call(API_SKIN_GETSKINNAME, (const char *)0);
}

inline const char *api_skin::getSkinPath() {
  return _call(API_SKIN_GETSKINPATH, (const char *)0);
}

inline const char *api_skin::getSkinsPath() {
  return _call(API_SKIN_GETSKINSPATH, (const char *)0);
}

inline const char *api_skin::getDefaultSkinPath() {
  return _call(API_SKIN_GETDEFAULTSKINPATH, (const char *)0);
}

inline ARGB32 *api_skin::imgldr_requestSkinBitmap(const char *file, int *has_alpha, int *x, int *y, int *subw, int *subh, int *w, int *h, int cached) {
  return _call(API_SKIN_IMGLDR_REQUESTSKINBITMAP, (ARGB32 *)NULL, file, has_alpha, x, y, subw, subh, w, h, cached);
}

inline void api_skin::imgldr_releaseSkinBitmap(ARGB32 *bmpbits) {
  _voidcall(API_SKIN_IMGLDR_RELEASESKINBITMAP, bmpbits);
}

inline COLORREF api_skin::filterSkinColor(COLORREF color, const char *elementid, const char *groupname) {
  return _call(API_SKIN_FILTERSKINCOLOR, (COLORREF)NULL, color, elementid, groupname);
}

inline void api_skin::reapplySkinFilters() {
  _voidcall(API_SKIN_REAPPLYSKINFILTERS);
}

inline int api_skin::colortheme_getNumColorSets() {
  return _call(API_SKIN_COLORTHEME_GETNUMCOLORSETS, (int)0);
}

inline const char *api_skin::colortheme_enumColorSet(int n) {
  return _call(API_SKIN_COLORTHEME_ENUMCOLORSET, (const char *)0, n);
}

inline int api_skin::colortheme_getNumColorGroups(const char *colorset) {
  return _call(API_SKIN_COLORTHEME_GETNUMCOLORGROUPS, (int)0, colorset);
}

inline const char *api_skin::colortheme_enumColorGroupName(const char *colorset, int n) {
  return _call(API_SKIN_COLORTHEME_ENUMCOLORGROUPNAME, (const char *)0, colorset, n);
}

inline ColorThemeGroup *api_skin::colortheme_enumColorGroup(int colorset, int n) {
  return _call(API_SKIN_COLORTHEME_ENUMCOLORGROUP, (ColorThemeGroup *)NULL, colorset, n);
}

inline ColorThemeGroup *api_skin::colortheme_getColorGroup(const char *colorset, const char *group) {
  return _call(API_SKIN_COLORTHEME_GETCOLORGROUP, (ColorThemeGroup *)NULL, colorset, group);
}

inline void api_skin::colortheme_setColorSet(const char *colorset) {
  _voidcall(API_SKIN_COLORTHEME_SETCOLORSET, colorset);
}

inline const char *api_skin::colortheme_getColorSet() {
  return _call(API_SKIN_COLORTHEME_GETCOLORSET, (const char *)0);
}

inline void api_skin::colortheme_newColorSet(const char *set) {
  _voidcall(API_SKIN_COLORTHEME_NEWCOLORSET, set);
}

inline void api_skin::colortheme_updateColorSet(const char *set) {
  _voidcall(API_SKIN_COLORTHEME_UPDATECOLORSET, set);
}

inline void api_skin::colortheme_renameColorSet(const char *set, const char *newname) {
  _voidcall(API_SKIN_COLORTHEME_RENAMESET, set, newname);
}

inline void api_skin::colortheme_deleteColorSet(const char *set) {
  _voidcall(API_SKIN_COLORTHEME_DELETE, set);
}


inline int api_skin::loadSkinFile(const char *xmlfile) {
  return _call(API_SKIN_LOADSKINFILE, (int)0, xmlfile);
}

inline void api_skin::unloadSkinPart(int skinpartid) {
  _voidcall(API_SKIN_UNLOADSKINPART, skinpartid);
}

inline RootWnd *api_skin::group_create(const char *groupid, int scripts_enabled) {
  return _call(API_SKIN_GROUP_CREATE, (RootWnd *)NULL, groupid, scripts_enabled);
}

inline int api_skin::group_exists(const char *groupid) {
  return _call(API_SKIN_GROUP_EXISTS, 1, groupid);
}

#ifdef WASABI_COMPILE_CONFIG
inline RootWnd *api_skin::group_create_cfg(const char *groupid, CfgItem *cfgitem, const char *attributename, int scripts_enabled) {
  return _call(API_SKIN_GROUP_CREATE_CFG, (RootWnd *)NULL, groupid, cfgitem, attributename, scripts_enabled);
}
#endif // WASABI_COMPILE_CONFIG

#ifdef WASABI_COMPILE_WNDMGR
inline RootWnd *api_skin::group_create_layout(const char *groupid, int scripts_enabled) {
  return _call(API_SKIN_GROUP_CREATE_LAYOUT, (RootWnd *)NULL, groupid, scripts_enabled);
}
#endif //WASABI_COMPILE_WNDMGR

inline int api_skin::group_destroy(RootWnd *group) {
  return _call(API_SKIN_GROUP_DESTROY, (int)0, group);
}

inline int api_skin::parse(const char *str, const char *how) {
  return _call(API_SKIN_PARSE, (int)0, str, how);
}

inline GuiObject *api_skin::xui_new(const char *classname) {
  return _call(API_SKIN_XUI_NEW, (GuiObject *)NULL, classname);
}

inline void api_skin::xui_delete(GuiObject *o) {
  _voidcall(API_SKIN_XUI_DELETE, o);
}

inline OSCURSOR api_skin::cursor_request(const char *id) {
  return _call(API_SKIN_CURSOR_REQUEST, (OSCURSOR)NULL, id);
}

inline int api_skin::getNumGroupDefs() {
  return _call(API_SKIN_GETNUMGROUPS, (int)0);
}

inline SkinItem *api_skin::enumGroupDef(int n) {
  return _call(API_SKIN_ENUMGROUP, (SkinItem *)NULL, n);
}

inline RootWnd *api_skin::group_createBySkinItem(SkinItem *item, int scripts_enabled) {
  return _call(API_SKIN_GROUP_CREATEBYITEM, (RootWnd *)NULL, item, scripts_enabled);
}

inline SkinItem *api_skin::getGroupDefAncestor(SkinItem *item) {
  return _call(API_SKIN_GETGROUPANCESTOR, (SkinItem *)NULL, item);
}

inline int api_skin::groupdef_getNumObjects(SkinItem *groupitem) {
  return _call(API_SKIN_GROUPDEF_GETNUMOBJECTS, 0, groupitem);
}

inline SkinItem *api_skin::groupdef_enumObject(SkinItem *item, int n) {
  return _call(API_SKIN_GROUPDEF_ENUMOBJECT, (SkinItem *)NULL, item, n);
}

inline int api_skin::loadGroupDefData(const char *groupdef, SkinItem **lastgroupitem) {
  return _call(API_SKIN_LOADGROUPDEFDATA, -1, groupdef, lastgroupitem);
}

inline void api_skin::skin_setLockUI(int l) {
  _voidcall(API_SKIN_SETLOCKUI, l);
}

inline int api_skin::skin_getLockUI() {
  return _call(API_SKIN_GETLOCKUI, 0);
}

inline double api_skin::skin_getVersion() {
  return _call(API_SKIN_GETVERSION, 0.8);
}

inline COLORREF api_skin::skin_getBitmapColor(const char *bitmapid) {
  return _call(API_SKIN_GETBITMAPCOLOR, 0xFFFF00FF, bitmapid);
}

class api_skinI : public api_skin {
  protected:
    api_skinI() {}
    virtual ~api_skinI() {}

  public:
    virtual COLORREF skin_getColorElement(char *type, const char **color_group = NULL)=0;
    virtual COLORREF *skin_getColorElementRef(char *type, const char **color_group = NULL)=0;
    virtual int *skin_getIterator()=0;
    virtual void skin_switchSkin(const char *skin_name, const char *skin_path=NULL)=0;
    virtual void skin_unloadSkin()=0;
    virtual const char *getSkinName()=0;
    virtual const char *getSkinPath()=0;
    virtual const char *getSkinsPath()=0;
    virtual const char *getDefaultSkinPath()=0;
    virtual ARGB32 *imgldr_requestSkinBitmap(const char *file, int *has_alpha, int *x, int *y, int *subw, int *subh, int *w, int *h, int cached)=0;
    virtual void imgldr_releaseSkinBitmap(ARGB32 *bmpbits)=0;
    virtual COLORREF filterSkinColor(COLORREF color, const char *elementid, const char *groupname)=0;
    virtual void reapplySkinFilters()=0;
    virtual int colortheme_getNumColorSets()=0;
    virtual const char *colortheme_enumColorSet(int n)=0;
    virtual int colortheme_getNumColorGroups(const char *colorset)=0;
    virtual const char *colortheme_enumColorGroupName(const char *colorset, int n)=0;
    virtual ColorThemeGroup *colortheme_enumColorGroup(int colorset, int n)=0;
    virtual ColorThemeGroup *colortheme_getColorGroup(const char *colorset, const char *group)=0;
    virtual void colortheme_setColorSet(const char *colorset)=0;
    virtual const char *colortheme_getColorSet()=0;
    virtual void colortheme_newColorSet(const char *set)=0;
    virtual void colortheme_updateColorSet(const char *set)=0;
    virtual void colortheme_renameColorSet(const char *set, const char *newname)=0;
    virtual void colortheme_deleteColorSet(const char *set)=0;
    virtual int loadSkinFile(const char *xmlfile)=0;
    virtual int loadGroupDefData(const char *groupdef, SkinItem **lastgroupitem)=0;
    virtual void unloadSkinPart(int skinpartid)=0;
    virtual RootWnd *group_create(const char *groupid, int scripts_enabled=1)=0;
    virtual int group_exists(const char *groupid)=0;
#ifdef WASABI_COMPILE_CONFIG
    virtual RootWnd *group_create_cfg(const char *groupid, CfgItem *cfgitem, const char *attributename, int scripts_enabled=1)=0;
#endif // WASABI_COMPILE_CONFIG
#ifdef WASABI_COMPILE_WNDMGR
    virtual RootWnd *group_create_layout(const char *groupid, int scripts_enabled=1)=0;
#endif //WASABI_COMPILE_WNDMGR
    virtual int group_destroy(RootWnd *group)=0;
    virtual int parse(const char *str, const char *how)=0;
    virtual GuiObject *xui_new(const char *classname)=0;
    virtual void xui_delete(GuiObject *o)=0;
    virtual OSCURSOR cursor_request(const char *id)=0;
    virtual int getNumGroupDefs()=0;
    virtual SkinItem *enumGroupDef(int n)=0;
    virtual RootWnd *group_createBySkinItem(SkinItem *item, int scripts_enabled=1)=0;
    virtual SkinItem *getGroupDefAncestor(SkinItem *groupitem)=0;
    virtual int groupdef_getNumObjects(SkinItem *groupitem)=0;
    virtual SkinItem *groupdef_enumObject(SkinItem *groupitem, int n)=0;
    virtual void skin_setLockUI(int l)=0;
    virtual int skin_getLockUI()=0;
    virtual double skin_getVersion()=0;
    virtual COLORREF skin_getBitmapColor(const char *id)=0;

  protected:
    RECVS_DISPATCH;
};

// {F2398F09-63B0-4442-86C9-F8BC473F6DA7}
static const GUID skinApiServiceGuid = 
{ 0xf2398f09, 0x63b0, 0x4442, { 0x86, 0xc9, 0xf8, 0xbc, 0x47, 0x3f, 0x6d, 0xa7 } };

extern api_skin *skinApi;


#endif
