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
#include "skinapi.h"
#include "../../studio/skin.h"
#include "../../studio/skinembed.h"
#include "../../studio/skinelem.h"
#include "../../studio/imgldr.h"
#include "../../studio/gammamgr.h"
#include "../../studio/groupmgr.h"
#include "../../studio/cursormgr.h"
#include "../../studio/guitree.h"
#include "../../studio/groupwndcreate.h"
#include "../../studio/autopopup.h"

api_skin *skinApi;
static waServiceTSingle<svc_windowCreate, GroupWndCreateSvc> groupWndCreate;

SkinApi::SkinApi() {
  lockui = 0;
  tha = new Skin();
  SkinParser::initialize();

  WASABI_API_SVC->service_register(&groupWndCreate);

  SkinElementsMgr::init();
  GammaMgr::init();

  String apppath = WASABI_API_APP->path_getAppPath();
  if (!Std::isDirChar(apppath.lastChar())) apppath.cat(DIRCHARSTR);

  skinspath = StringPrintf("%s%s%s", apppath, WASABI_SKINS_SUBDIRECTORY, DIRCHARSTR);
}

SkinApi::~SkinApi() {
  delete tha; tha = NULL;
  #ifdef WASABI_COMPILE_WNDMGR
  AutoPopup::reset();
  #endif
  SkinElementsMgr::deinit();
  GammaMgr::deinit();
  WASABI_API_SVC->service_deregister(&groupWndCreate);
  SkinParser::shutdown();
}

void SkinApi::preShutdown() {
  Skin::unloadSkin();
  SkinElementsMgr::resetSkinElements();
}

COLORREF SkinApi::skin_getColorElement(char *type, const char **color_group) {
  return SkinElementsMgr::getColorElement(type, color_group);
}

COLORREF *SkinApi::skin_getColorElementRef(char *type, const char **color_group) {
  return SkinElementsMgr::getColorElementRef(type, color_group);
}

int *SkinApi::skin_getIterator() {
  return Skin::getSkinPartIteratorPtr();
}

void SkinApi::skin_switchSkin(const char *skin_name, const char *skin_path) {
  if (skin_name && *skin_name) Skin::toggleSkin(skin_name, skin_path, 1);
}

void SkinApi::skin_unloadSkin() {
  Skin::unloadSkin();
}

const char *SkinApi::getSkinName() {
  return Skin::getSkinName(); 
}

const char *SkinApi::getSkinPath() {
  return Skin::getSkinPath(); 
}

const char *SkinApi::getSkinsPath() {
  return skinspath;
}

const char *SkinApi::getDefaultSkinPath() {
  return Skin::getDefaultSkinPath(); 
}

ARGB32 *SkinApi::imgldr_requestSkinBitmap(const char *file, int *has_alpha, int *x, int *y, int *subw, int *subh, int *w, int *h, int cached) {
  if (file == NULL) {
    DebugString("illegal param : file == NULL\n");
    return NULL;
  }
  return imageLoader::requestSkinBitmap(file, has_alpha, x, y, subw, subh, w, h, cached);
}

void SkinApi::imgldr_releaseSkinBitmap(ARGB32 *bmpbits) {
  if (bmpbits == NULL) {
    DebugString("illegal param : bmpbits == NULL\n");
    return;
  }
  imageLoader::releaseSkinBitmap(bmpbits);
}

COLORREF SkinApi::filterSkinColor(COLORREF color, const char *elementid, const char *groupname) {
  return imageLoader::filterSkinColor(color, elementid, groupname);
}

void SkinApi::reapplySkinFilters() {
  imageLoader::applySkinFilters();
}

int SkinApi::colortheme_getNumColorSets() {
  return GammaMgr::getNumGammaSets();
}

const char *SkinApi::colortheme_enumColorSet(int n) {
  return GammaMgr::enumGammaSet(n);
}

int SkinApi::colortheme_getNumColorGroups(const char *colorset) {
  return GammaMgr::getNumGammaGroups(colorset);
}

const char *SkinApi::colortheme_enumColorGroupName(const char *colorset, int n) {
  return GammaMgr::enumGammaGroup(colorset, n);
}

ColorThemeGroup *SkinApi::colortheme_enumColorGroup(int colorset, int colorgroup) {
  return GammaMgr::enumColorThemeGroup(colorset, colorgroup);
}

ColorThemeGroup *SkinApi::colortheme_getColorGroup(const char *colorset, const char *colorgroup) {
  return GammaMgr::getColorThemeGroup(colorset, colorgroup);
}

void SkinApi::colortheme_setColorSet(const char *colorset) {
  GammaMgr::setGammaSet(colorset);
}

const char *SkinApi::colortheme_getColorSet() {
  return GammaMgr::getGammaSet();
}

void SkinApi::colortheme_newColorSet(const char *set) {
  GammaMgr::newGammaSet(set);
}

void SkinApi::colortheme_updateColorSet(const char *set) {
  GammaMgr::updateGammaSet(set);
}

void SkinApi::colortheme_renameColorSet(const char *set, const char *newname) {
  GammaMgr::renameGammaSet(set, newname);
}

void SkinApi::colortheme_deleteColorSet(const char *set) {
  GammaMgr::deleteGammaSet(set);
}

int SkinApi::loadSkinFile(const char *xmlfile) {
  return Skin::loadSkinPart(xmlfile);
}

void SkinApi::unloadSkinPart(int skinpartid) {
  Skin::unloadSkinPart(skinpartid);
}

RootWnd *SkinApi::group_create(const char *groupid, int scripts_enabled) {
  return GroupMgr::instantiate(groupid, GROUP_GROUP, NULL, scripts_enabled);
}

int SkinApi::group_destroy(RootWnd *group) {
  return GroupMgr::destroy(static_cast<Group*>(group));
}

int SkinApi::group_exists(const char *groupid) {
  return GroupMgr::exists(groupid);
}

#ifdef WASABI_COMPILE_CONFIG
RootWnd *SkinApi::group_create_cfg(const char *groupid, CfgItem *cfgitem, const char *attributename, int scripts_enabled) {
  return GroupMgr::instantiate(groupid, cfgitem, attributename, scripts_enabled);
}
#endif // WASABI_COMPILE_CONFIG

#ifdef WASABI_COMPILE_WNDMGR
RootWnd *SkinApi::group_create_layout(const char *groupid, int scripts_enabled) {
  return GroupMgr::instantiate(groupid, GROUP_LAYOUTGROUP, NULL, scripts_enabled);
}
#endif //WASABI_COMPILE_WNDMGR

OSCURSOR SkinApi::cursor_request(const char *id) {
  return CursorMgr::requestCursor(id);
}

int SkinApi::parse(const char *str, const char *how) {
  return SkinParser::parse(str, how);
}

GuiObject *SkinApi::xui_new(const char *classname) {
  return SkinParser::xui_new(classname);
}

void SkinApi::xui_delete(GuiObject *o) {
  SkinParser::xui_delete(o);
}

int SkinApi::getNumGroupDefs() {
  return guiTree->getNumGroupDefs();
}

SkinItem *SkinApi::enumGroupDef(int n) {
  return guiTree->enumGroupDef(n);
}

RootWnd *SkinApi::group_createBySkinItem(SkinItem *item, int scripts_enabled) {
  return GroupMgr::instantiate(NULL, GROUP_GROUP, item, scripts_enabled);
}

SkinItem *SkinApi::getGroupDefAncestor(SkinItem *item) {
  return guiTree->getGroupDefAncestor(item);
}

int SkinApi::groupdef_getNumObjects(SkinItem *_item) {
  GuiTreeItem *item = static_cast<GuiTreeItem *>(_item);
  int idx = item->getIdx();
  idx++;
  int n = 0;
  while (1) {
    GuiTreeItem *it = guiTree->getList()->enumItem(idx);
    if (it->getType() == XML_TAG_GROUPDEF && it->getParams() == NULL) break;
    idx++; n++;
  }
  return n;
}

SkinItem *SkinApi::groupdef_enumObject(SkinItem *_item, int n) {
  GuiTreeItem *item = static_cast<GuiTreeItem *>(_item);
  int idx = item->getIdx();
  idx++;
  int _n = 0;
  GuiTreeItem *it = NULL;
  while (1) {
    it = guiTree->getList()->enumItem(idx);
    if (it->getType() == XML_TAG_GROUPDEF && it->getParams() == NULL) break;
    if (n == _n) break;
    idx++; _n++;
  }
  return it;
}

int SkinApi::loadGroupDefData(const char *groupdef, SkinItem **lastgroupdef) {
  String s;
  s = "buf:";
  s += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
       "<WinampAbstractionLayer version=\"0.8\">";

  s += groupdef;

  s += "</WinampAbstractionLayer>";

  int r = Skin::loadSkinPart(s);
  if (lastgroupdef != NULL) 
    *lastgroupdef = guiTree->getLastDefinedGroup();
  return r;
}


double SkinApi::skin_getVersion() {
  return SkinParser::getSkinVersion();
}

COLORREF SkinApi::skin_getBitmapColor(const char *id) {
  SkinBitmap bitmap(id);
  BltCanvas c(bitmap.getWidth()+1, bitmap.getHeight()+1);
  bitmap.blit(&c, 0, 0);
  int x = bitmap.getWidth()/2;
  int y = bitmap.getHeight()/2;
  int *bits = (int *)c.getBits();
  if (bits != NULL) {
    return bits[x+y*bitmap.getWidth()+1];
  }
  return 0xFFFF00FF;
}
