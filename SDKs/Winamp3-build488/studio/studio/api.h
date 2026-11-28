/*

  Nullsoft WASABI Source File License

  Copyright 1999-2001 Nullsoft, Inc.

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


  Brennan Underwood
  brennan@nullsoft.com

*/

#ifndef _API_H
#define _API_H

#include "../common/dispatch.h"
#include "../common/platform/platform.h"	// basic type defs
#include "../common/script/vcputypes.h"
#include "../bfc/console.h"

#define LATEST_API_VERSION	1

class CanvasBase;	// see ../common/canvas.h
class CfgItem;		// see ../attribs/cfgitem.h
class CoreCallback;	// see corecb.h
class EmbedWnd;		// see ../common/embedwnd.h
class ItemSequencer;	// see ../common/sequence.h
class RootWnd;		// see ../common/rootwnd.h
class SkinFilter;	// see skinfilter.h
class SysCallback;	// see syscb.h
class XmlReaderCallback; // see xmlreader.h
class waServiceFactory;	// see service.h
class MetricsCallback;	// see metricscb.h
class ScriptObjectController; // see objcontroller.h
class dbSvcScanner; // see ../studio/services/svc_db.h
class TimerClient; // see ../bfc/timerclient.h
class WindowHolder; // see ../bfc/wnds/wndholder.h
class PopupExitCallback; // see ../bfc/popexitcb.h
class GuiObject; // see ../common/script/guiobject.h

typedef unsigned int CoreToken;
typedef unsigned long ARGB32;
class WaComponent;

RootWnd *const MODALWND_NOWND = reinterpret_cast<RootWnd*>(-1);

class ComponentAPI : public Dispatchable {
public:
  int getVersion();	// the API version #

// studio.exe info
  // text string of version
  const char *main_getVersionString();
  // not-very-reliable auto-incremented build number... mostly just interesting
  unsigned int main_getBuildNumber();
  // the GUID of studio
  GUID main_getGUID();
  // the HINSTANCE (try not to abuse :)
  HINSTANCE main_gethInstance();
  // the icon
  HICON main_getIcon(int _small);
  // the master window
  RootWnd *main_getRootWnd();
  // the command line
  const char *main_getCommandLine();
  // the window title
  void main_setWindowTitle(const char *title);


// config file stuff
  void setIntPrivate(const char *name, int val);
  int getIntPrivate(const char *name, int def_val);
  void setIntArrayPrivate(const char *name, const int *val, int nval);
  int getIntArrayPrivate(const char *name, int *val, int nval);
  void setStringPrivate(const char *name, const char *str);
  int getStringPrivate(const char *name, char *buf, int buf_len, const char *default_str);
  int getStringPrivateLen(const char *name);
  void setIntPublic(const char *name, int val);
  int getIntPublic(const char *name, int def_val);
  void setStringPublic(const char *name, const char *str);
  int getStringPublic(const char *name, char *buf, int buf_len, const char *default_str);


// playback core
// those functions are DEPRECATED. use the CoreHandle class instead. THEY WILL GET AWAY SOON! :)
  const char *core_getSupportedExtensions();//just the *.mp3 or whatever
  const char *core_getExtSupportedExtensions();// including names
  CoreToken core_create(); // returns a token
  int core_free(CoreToken core);
  int core_setNextFile(CoreToken core, const char *playstring);
  int core_getStatus(CoreToken core);	// returns -1 if paused, 0 if stopped and 1 if playing
  const char *core_getCurrent(CoreToken core);
  int core_getCurPlaybackNumber(CoreToken core);
  int core_getPosition(CoreToken core);
  int core_getWritePosition(CoreToken core);
  int core_setPosition(CoreToken core, int ms);
  int core_getLength(CoreToken core);
  // this method queries the core plugins directly, bypassing the db
  int core_getPluginData(const char *playstring, const char *name,
    char *data, int data_len, int data_type=0);	// returns size of data
  unsigned int core_getVolume(CoreToken core);
  void core_setVolume(CoreToken core, unsigned int vol);	// 0..255
  int core_getPan(CoreToken core);            // -127..127
  void core_setPan(CoreToken core, int bal);  // -127..127
  // register here for general callbacks in core status.
  void core_addCallback(CoreToken core, CoreCallback *cb);
  void core_delCallback(CoreToken core, CoreCallback *cb);
  // get visualization data, returns 0 if you should blank out
  int core_getVisData(CoreToken core, void *dataptr, int sizedataptr);
  int core_getLeftVuMeter(CoreToken core);
  int core_getRightVuMeter(CoreToken core);
  
  int core_registerSequencer(CoreToken core, ItemSequencer *seq);
  int core_deregisterSequencer(CoreToken core, ItemSequencer *seq);
  void core_userButton(CoreToken core, int button);	// see buttons.h

  int core_getEqStatus(CoreToken core); // returns 1 if on, 0 if off
  void core_setEqStatus(CoreToken core, int enable);  // 1=on, 0=off
  int core_getEqPreamp(CoreToken core);            // -127 to 127 (-20db to +20db)
  void core_setEqPreamp(CoreToken core, int pre);  // -127 to 127 (-20db to +20db)
  int core_getEqBand(CoreToken core, int band);            // band=0-9
  void core_setEqBand(CoreToken core, int band, int val);  // band=0-9,val=-127 to 127 (-20db to +20db)
  int core_getEqAuto(CoreToken core);     // returns 1 if on, 0 if off
  void core_setEqAuto(CoreToken core, int enable); // 1=on, 0=off
  void core_setCustomMsg(CoreToken core, const char *text); 
  void core_registerExtension(const char *extensions, const char *extension_name, const char *family=NULL);
  const char *core_getExtensionFamily(const char *extension);
  void core_unregisterExtension(const char *extensions);

// services
  int service_register(waServiceFactory *);
  int service_deregister(waServiceFactory *);
  int service_getNumServices(FOURCC svc_type);	// see common/svc_enum.h
  // enumerate by family
  waServiceFactory *service_enumService(FOURCC svc_type, int n);
  // fetch by GUID
  waServiceFactory *service_getServiceByGuid(GUID guid);
  // service owner calls this when it issues a service *
  int service_lock(waServiceFactory *owner, void *svcptr);
  // service client calls this when it uses a service *
  int service_clientLock(void *svcptr);
  // service client calls this when done w/ service *
  int service_release(void *svcptr);
  const char *service_getTypeName(FOURCC svc_type);
  GUID service_getOwningComponent(void *svcptr);
  GUID service_getLockingComponent(void *svcptr);

// sys callbacks
  int syscb_registerCallback(SysCallback *cb, void *param=NULL);
  int syscb_deregisterCallback(SysCallback *cb);

// component commands
  int cmd_sendCommand(GUID guid, char *command, int param1=0, int param2=0, void *ptr=NULL, int ptrlen=0);
  // this sends a command through the main wasabi thread
  int cmd_postCommand(GUID guid, char *command, int param1=0, int param2=0, void *ptr=NULL, int ptrlen=0, int waitforanswer=0);
  void cmd_broadcastCommand(char *command, int param1=0, int param2=0, void *ptr=NULL, int ptrlen=0);

  void status_setTrayTipText(const char *text);
  void main_enableInput(int enable);

  // language/locality support
  const char *locales_getTranslation(const char *str); // if not found, returns the str paramer
  void locales_addTranslation(const char *from, const char *to);
  const char *locales_getBindFromAction(int action);
  int locales_getNumEntries();
  const char *locales_enumEntry(int n);
  void locales_registerAcceleratorSection(const char *name, RootWnd *wnd, int global=0);

// configuration
  void config_registerCfgItem(CfgItem *cfgitem);
  void config_deregisterCfgItem(CfgItem *cfgitem);
  int config_getNumCfgItems();
  CfgItem *config_enumCfgItem(int n);
  CfgItem *config_getCfgItemByGuid(GUID g);

// message box
  int messageBox(const char *txt, const char *title, int flags, const char *not_anymore_identifier, RootWnd *parenwnt);
  RootWnd *getModalWnd();
  void pushModalWnd(RootWnd *w=MODALWND_NOWND);
  void popModalWnd(RootWnd *w=MODALWND_NOWND);

// size metrics
  void metrics_addCallback(MetricsCallback *mcb);
  void metrics_delCallback(MetricsCallback *mcb);
  int metrics_getDelta();
  int metrics_setDelta(int newdelta);

  void main_navigateUrl(const char *url);

// database stuff
  int metadb_optimizePlayString(char *playstring);
  int metadb_addNewItem(const char *playstring, const char *initial_name=NULL);
  int metadb_deleteItem(const char *playstring);
  void metadb_sync();

  // called by class Playstring
  const char *metadb_dupPlaystring(const char *playstring, int tablenum=0);
  void metadb_releasePlaystring(const char *playstring, int tablenum=0);

// metadata assistance from metaDB
  void metadb_getInfo(const char *playstring, char *info, int maxlen);
  int metadb_getLength(const char *playstring);
  int metadb_getMetaData(const char *playstring, const char *name,
    char *data, int data_len, int data_type=0); // returns size of data
  int metadb_getMetaDataType(const char *name);
  GUID metadb_getMetaDataTable(const char *name);
  int metadb_setMetaData(const char *playstring, const char *name,
    const char *data, int data_len, int data_type);

  // attempts to render data (from db or whatever) in given rect
  // if you use this you can show new datatypes your code doesn't understand
  int metadb_renderData(CanvasBase *cb, RECT &r, void *data, int datatype, int centered=FALSE);
  // attempts to convert a datatype to a text representation
  int metadb_convertToText(void *data, int datatype, char *buf, int buflen);

  // delete metadata for this service and playitem
  void metadb_deleteMetaData(GUID metadataservice, const char *playitem);
  // creates a scanner on a metadata service's table. not guaranteed to succeed (can be denied)
  dbSvcScanner *metadb_newScanner(GUID metadataservice);
  // deletes a scanner previously created by metadb_newScanner
  void metadb_deleteScanner(dbSvcScanner *scanner);

#if 1//REIMPLEMENT
// completeness graph
  void startCompleted(int max);
  void incCompleted();
  void endCompleted();

#endif
// skinning
  COLORREF skin_getColorElement(char *type);
  COLORREF *skin_getColorElementRef(char *type);
  int *skin_getIterator();	// an int that cycles on skin reloading

#define RenderBaseTexture renderBaseTexture //CUT

  void skin_renderBaseTexture(RootWnd *base, CanvasBase *c, RECT &r, RootWnd *destWnd, int alpha=255);
  void skin_registerBaseTextureWindow(RootWnd *window, const char *bmp=NULL);
  void skin_unregisterBaseTextureWindow(RootWnd *window);
  
  void skin_switchSkin(char *skin_name);
  const char *getSkinName();
  const char *getSkinPath();
  const char *getDefaultSkinPath();
  const char *getAppPath();

// generic malloc/free. Use these if you are returning data that other
// components will need to free (possibly after you have been unloaded)
  void *sysMalloc(int size);
  void sysFree(void *ptr);
  void *sysRealloc(void *ptr, int newsize);

// image loading
  ARGB32 *imgldr_makeBmp(const char *filename, int *has_alpha, int *w, int *h);
  ARGB32 *imgldr_makeBmp(HINSTANCE hInst, int id, int *has_alpha, int *w, int *h, const char *colorgroup=NULL);
  ARGB32 *imgldr_requestSkinBitmap(const char *file, int *has_alpha, int *x, int *y, int *subw, int *subh, int *w, int *h, int cached);
  void imgldr_releaseSkinBitmap(ARGB32 *bmpbits);

  COLORREF filterSkinColor(COLORREF, const char *groupname);
  void reapplySkinFilters();
  int colortheme_getNumColorSets();
  const char *colortheme_enumColorSet(int n);
  int colortheme_getNumColorGroups(const char *colorset);
  const char *colortheme_enumColorGroup(const char *colorset, int n);
  void colortheme_setColorSet(const char *colorset);
  const char *colortheme_getColorSet();

  void paintset_render(int set, CanvasBase *c, const RECT *r, int alpha=255);
  void paintset_renderTitle(char *t, CanvasBase *c, const RECT *r, int alpha=255);

// animated rects
  void drawAnimatedRects(const RECT *r1, const RECT *r2);

// Dde com
  void sendDdeCommand(const char *application, const char *command, DWORD minimumDelay);

// XML Reader
  void xmlreader_registerCallback(char *matchstr, XmlReaderCallback *callback);
  void xmlreader_unregisterCallback(XmlReaderCallback *callback);
  void xmlreader_loadFile(const char *filename, XmlReaderCallback *only_this_object=NULL);

// Window Tracker
  void wndTrackAdd(RootWnd *wnd);
  void wndTrackRemove(RootWnd *wnd);
  BOOL wndTrackDock(RootWnd *wnd, RECT *r, int mask);
  BOOL wndTrackDock(RootWnd *wnd, RECT *r, RECT *or, int mask);
  void wndTrackStartCooperative(RootWnd *wnd);
  void wndTrackEndCooperative(void);
  int wndTrackWasCooperative(void);
  void wndTrackInvalidateAll(void);

// Components
  int getNumComponents();
  GUID getComponentGUID(int c);
  const char *getComponentName(GUID componentGuid);
  CfgItem *compon_getCfgInterface(GUID componentGuid);
  int comp_notifyScripts(const char *s, int i1, int i2);
  int autopopup_registerGuid(GUID g, const char *desc, const char *prefered_container=NULL, int container_flag=0);
  int autopopup_registerGroupId(const char *groupid, const char *desc, const char *prefered_container=NULL, int container_flag=0);
  void autopopup_unregister(int id);

  int skinwnd_toggleByGuid(GUID g, const char *prefered_container=NULL, int container_flag=0, RECT *sourceanimrect=NULL, int transcient=0);
  int skinwnd_toggleByGroupId(const char *groupid, const char *prefered_container=NULL, int container_flag=0, RECT *sourceanimrect=NULL, int transcient=0);
  RootWnd *skinwnd_createByGuid(GUID g, const char *prefered_container=NULL, int container_flag=0, RECT *sourceanimrect=NULL, int transcient=0, int starthidden=0);
  RootWnd *skinwnd_createByGroupId(const char *groupid, const char *prefered_container=NULL, int container_flag=0, RECT *sourceanimrect=NULL, int transcient=0, int starthidden=0);
  void skinwnd_destroy(RootWnd *w, RECT *destanimrect=NULL);
  int skinwnd_getNumByGuid(GUID g);
  RootWnd *skinwnd_enumByGuid(GUID g, int n);
  int skinwnd_getNumByGroupId(const char *groupid);
  RootWnd *skinwnd_enumByGroupId(const char *groupid, int n);
  void skinwnd_attachToSkin(RootWnd *w, int side, int size);

  ScriptObject *skin_getContainer(const char *container_name);
  ScriptObject *skin_getLayout(ScriptObject *container, const char *layout_name);

  void wndholder_register(WindowHolder *wh);
  void wndholder_unregister(WindowHolder *wh);

  // allows a rootWndFromPoint that won't get other instances RootWnds
  // also, this will find VirtualWnds while the WIN32 version won't
  // and... this is portable :)
  RootWnd *rootWndFromPoint(POINT *pt);
  void registerRootWnd(RootWnd *wnd);
  void unregisterRootWnd(RootWnd *wnd);
  int rootwndIsValid(RootWnd *wnd);

  //keyboard support
  int forwardOnChar(unsigned int c);
  int forwardOnKeyDown(RootWnd *from, int k);
  int forwardOnKeyUp(int k);
  int forwardOnSysKeyDown(RootWnd *from, int k, int kd);
  int forwardOnSysKeyUp(int k, int kd);
  int forwardOnKillFocus();

  // file readers support
  void *fileOpen(const char *filename, const char *mode);
  void fileClose(void *fileHandle);
  int fileRead(void *buffer, int size, void *fileHandle);
  int fileWrite(const void *buffer, int size, void *fileHandle);
  int fileSeek(int offset, int origin, void *fileHandle);
  int fileTell(void *fileHandle);
  int fileGetFileSize(void *fileHandle);
  int fileExists(const char *filename);
  int fileRemove(const char *filename);
  // can't move directories between volumes on win32 :P
  int fileMove(const char *filename, const char *destfilename);
  
// text output functions. don't call these directly, use the Canvas stuff
  void font_textOut(CanvasBase *c, int style, int x, int y, int w, int h, const char *txt);
  int font_getInfo(CanvasBase *c, const char *font, int infoid, const char *txt, int *w, int *h);

// call this after deallocating a lot of RAM
  void hint_garbageCollect();
// outputs a message to the console, severity 0 to 9 (critical, before terminating operations)
  int console_outputString(int severity, const char *string);

// loads a skin file, but does not switch main skin.
// use this to load a custom xml skin/script/definition file or package (using includes)
// either before (if you want to allow the content to be overriden) or after
// skin loading
// return value is what you need to give to unloadSkinPart() to clean up your mess
  int loadSkinFile(const char *xmlfile);
  void unloadSkinPart(int skinpart);

  void maki_pushObject(void *o);
  void maki_pushInt(int v);
  void maki_pushBoolean(int b);
  void maki_pushFloat(float f);
  void maki_pushDouble(double d);
  void maki_pushString(const char *s);
  void maki_pushVoid();
  void maki_pushAny(scriptVar v);
  void *maki_popObject();
  int maki_popInt();
  int maki_popBoolean();
  float maki_popFloat();
  double maki_popDouble();
  const char *maki_popString();
  scriptVar maki_popAny();
  void maki_popDiscard();
  const char *maki_getFunction(int dlfid, int *n, ScriptObjectController **p);
  int maki_addDlfRef(ScriptObjectController *o, const char *function_name, void *host);
  void maki_addDlfClassRef(ScriptObjectController *o, void *host);
  void maki_remDlfRef(void *host);
  scriptVar maki_callFunction(ScriptObject *o, int dlfid, scriptVar **params);
  scriptVar maki_triggerEvent(ScriptObject *o, int dlfid, int np, int scriptid=-1);
  int maki_getScriptInt(scriptVar v);
  int maki_getScriptBoolean(scriptVar v);
  float maki_getScriptFloat(scriptVar v);
  double maki_getScriptDouble(scriptVar v);
  const char *maki_getScriptString(scriptVar v);
  ScriptObject *maki_getScriptObject(scriptVar v);
  scriptVar maki_updateDlf(maki_cmd *cmd, int *dlfid, int *linkcount);
  ScriptObject *maki_instantiate(GUID classguid);
  void maki_destroy(ScriptObject *o);
  void *maki_encapsulate(GUID classguid, ScriptObject *o);
  void maki_deencapsulate(GUID classguid, void *o);
  ScriptObjectController *maki_getController(GUID scriptclass);
  int maki_createOrphan(int type);
  void maki_killOrphan(int id);
  void maki_setObjectAtom(const char *atomname, ScriptObject *object);
  ScriptObject *maki_getObjectAtom(const char *atomname);
  ScriptObject *maki_findObject(const char *name);
  void vcpu_addScriptObject(ScriptObject *o);
  void vcpu_removeScriptObject(ScriptObject *o);
  int vcpu_getCacheCount();
  int vcpu_isValidScriptId(int id);
  int vcpu_mapVarId(int varid, int scriptid);
  int vcpu_getUserAncestorId(int varid, int scriptid);
  int vcpu_getNumEvents();
  int vcpu_getEvent(int event, int *dlf, int *script, int *var);

// groups creation / destruction

  RootWnd *group_create(const char *groupid);
  RootWnd *group_create_layout(const char *groupid);
  RootWnd *group_create_cfg(const char *groupid, CfgItem *cfgitem, const char *attributename);
  int group_destroy(RootWnd *group);

// URL Launching Capabilities
  void navigateUrl(const char *url); 

// translates @vars@
  const char *varmgr_translate(const char *str);

  int parse(const char *str, const char *how);

  void timer_add(TimerClient *client, int id, int ms);
  void timer_remove(TimerClient *client, int id=-1);

  void popupexit_deregister(PopupExitCallback *cb);
  void popupexit_register(PopupExitCallback *cb, RootWnd *watched);
  int popupexit_check(RootWnd *w);
  void popupexit_signal();

  void preferences_registerGroup(const char *group_id, const char *desc, GUID pref_guid, GUID pref_guid_parent=INVALID_GUID);
  int preferences_getNumGroups();
  const char *preferences_enumGroup(int n, const char **desc, GUID *this_guid, GUID *parent_guid);

  void appdeactivation_push_disallow(RootWnd *w);
  void appdeactivation_pop_disallow(RootWnd *w);
  int appdeactivation_isallowed(RootWnd *w);
  void appdeactivation_setbypass(int i);

  GuiObject *xui_new(const char *classname);
  void xui_delete(GuiObject *o);

#define hintGarbageCollect hint_garbageCollect	//FUCKO CUT

  // Dispatchable codes
  enum {
    GETVERSION		= 100,
    GETVERSIONSTRING	= 101,
    GETBUILDNUMBER	= 102,
    GETGUID		= 103,
    GETHINSTANCE	= 104,
    GETICON		= 105,
    GETROOTWND		= 106,
    SETINTPRIVATE	= 200,
    GETINTPRIVATE	= 201,
    SETINTARRAYPRIVATE	= 202,
    GETINTARRAYPRIVATE	= 203,
    SETSTRINGPRIVATE	= 204,
    GETSTRINGPRIVATE	= 205,
    SETSTRINGPUBLIC	= 206,
    GETSTRINGPUBLIC	= 207,
    GETSTRINGPRIVATELEN	= 208,

    COREGETSUPPORTEDEXTENSIONS		= 300,
    COREGETEXTSUPPORTEDEXTENSIONS	= 301,
    CORECREATE				= 302,
    COREFREE				= 303,
    CORESETNEXTFILE			= 306,
    COREGETSTATUS			= 307,
    COREGETCURRENT			= 308,
    COREGETPOSITION			= 309,
    CORESETPOSITION			= 310,
    COREGETLENGTH			= 311,
    COREGETPLUGINDATA			= 312,
    COREGETVOLUME			= 313,
    CORESETVOLUME			= 314,
    COREADDCALLBACK			= 315,
    COREDELCALLBACK			= 316,
    COREGETVISDATA			= 317,
    COREGETLEFTVUMETER			= 318,
    COREGETRIGHTVUMETER			= 319,
    COREREGISTERSEQUENCER		= 320,
    COREDEREGISTERSEQUENCER		= 321,
    COREUSERBUTTON			= 322,
    COREGETEQSTATUS			= 323,
    CORESETEQSTATUS			= 324,
    COREGETEQPREAMP			= 325,
    CORESETEQPREAMP			= 326,
    COREGETEQBAND			= 327,
    CORESETEQBAND			= 328,
    COREREGISTEREXTENSION = 7030,	// 329 retired
    COREGETEXTENSIONFAMILY = 7040,
    COREUNREGISTEREXTENSION = 330,
    COREGETEQAUTO     = 331,
    CORESETEQAUTO     = 332,
    COREGETPAN        = 333,
    CORESETPAN        = 334,
    CORESETCUSTOMMSG  = 335,
    COREGETCURPLAYBACKNUMBER = 336,

    SERVICEREGISTER			= 400,
    SERVICEDEREGISTER			= 401,
    SERVICEGETNUMSERVICES		= 402,
    SERVICEENUMSERVICE			= 403,
    SERVICELOCK				= 404,
    SERVICERELEASE			= 405,
    SERVICEGETTYPENAME			= 406,
    SERVICEGETOWNINGCOMPONENT		= 407,
    SERVICEGETLOCKINGCOMPONENT		= 408,
    SERVICECLIENTLOCK			= 409,
    SERVICEGETSERVICEBYGUID		= 410,

    PUSHOBJECT = 1100,
    PUSHINT = 1101,
    PUSHBOOLEAN = 1102,
    PUSHFLOAT = 1103,
    PUSHDOUBLE = 1104,
    PUSHSTRING = 1105,
    PUSHVOID = 1106,
    PUSHANY = 1107,

    POPOBJECT = 1120,
    POPINT = 1121,
    POPBOOLEAN = 1122,
    POPFLOAT = 1123,
    POPDOUBLE = 1124,
    POPSTRING = 1125,
    POPDISCARD = 1126,
    POPANY = 1127,

    GETMAKIFUNCTION = 1128,
    ADDDLFREF = 1129,
    REMDLFREF = 1130,
    CALLFUNCTION = 1131,
    ADDDLFCLASSREF = 1132,

    TRIGGEREVENT = 1140,
    UPDATEDLF = 1141,

    GETSCRIPTOBJECT = 1160,
    GETSCRIPTINT = 1161,
    GETSCRIPTBOOLEAN = 1162,
    GETSCRIPTFLOAT = 1163,
    GETSCRIPTDOUBLE = 1164,
    GETSCRIPTSTRING = 1165,

    GETCACHECOUNT = 1166,
    ISVALIDSCRIPTID = 1167,
    MAPVARID = 1168,
    GETUSERANCESTORID = 1169,
    GETNUMEVENTS = 1170,
    GETEVENT = 1171,

    ADDSCRIPTOBJECT = 1172,
    REMOVESCRIPTOBJECT = 1173,

    MAKIINSTANTIATE = 1174,
    MAKIDESTROY = 1175,

    CREATEORPHAN = 1176,
    KILLORPHAN = 1177,
    MAKIGETCONTROLLER=1178,

    MAKIENCAPSULATE = 1179,
    MAKIDEENCAPSULATE = 1180,

    MAKIGETOBJATOM = 1181,
    MAKISETOBJATOM = 1182,

    MAKIFINDOBJECT = 1185,

    GROUPCREATE = 1200,
    GROUPDESTROY = 1201,
    GROUPCREATECFG = 1202,
    GROUPCREATELAYOUT = 1203,

    AUTOPOPUP_REGGUID = 1310,
    AUTOPOPUP_REGGROUP = 1311,
    AUTOPOPUP_UNREG = 1312,

    SYSCB_REGISTERCALLBACK=1400,
    SYSCB_DEREGISTERCALLBACK=1410,
    CMD_SENDCOMMAND=1600,
    CMD_BROADCASTCOMMAND=1610,
    CMD_POSTCOMMAND=1620,
    STATUS_SETTRAYTIPTEXT=1830,
    MAIN_ENABLEINPUT=1900,
    LOCALES_GETTRANSLATION=2000,
    LOCALES_ADDTRANSLATION=2010,
    LOCALES_GETBINDFROMACTION=2020,
    LOCALES_GETNUMENTRIES=2030,
    LOCALES_ENUMENTRY=2040,
    LOCALES_REGISTERACCELERATORSECTION=2050,
//retired    TIMEFMT_PRINTMINSEC=2100,
//retired    TIMEFMT_PRINTTIMESTAMP=2110,
    MESSAGEBOX=2300,
    GETMODALWND=2400,
    PUSHMODALWND=2410,
    POPMODALWND=2420,
    METRICS_ADDCALLBACK=2500,
    METRICS_DELCALLBACK=2510,
    METRICS_GETDELTA=2520,
    METRICS_SETDELTA=2530,
    MAIN_NAVIGATEURL=2620,
    METADB_OPTIMIZEPLAYSTRING=2700,
    METADB_ADDNEWITEM=2710,
    METADB_DELETEITEM=2715,
    METADB_SYNC=2720,
    METADB_DUPPLAYSTRING=2730,
    METADB_RELEASEPLAYSTRING=2740,
    METADB_GETINFO=2750,
    METADB_GETLENGTH=2760,
    METADB_GETMETADATA=2770,
    METADB_GETMETADATATYPE=2780,
    METADB_GETMETADATATABLE=2781,
    METADB_SETMETADATA=2790,
    METADB_RENDERDATA=2800,
    METADB_CONVERTTOTEXT=2810,
    METADB_DELETEMETADATA=2820,
    METADB_NEWSCANNER=2830,
    METADB_DELETESCANNER=2840,
    STARTCOMPLETED=3200,
    INCCOMPLETED=3210,
    ENDCOMPLETED=3220,    
    SKIN_GETELEMENTCOLOR=3300,
    SKIN_GETELEMENTREF=3310,
    SKIN_GETITERATOR=3320,
    SKIN_RENDERBASETEXTURE=3330,
    SKIN_REGISTERBASETEXTUREWINDOW=3340,
    SKIN_UNREGISTERBASETEXTUREWINDOW=3350,
    SKIN_SWITCHSKIN=3360,
    GETSKINNAME=3400,
    GETSKINPATH=3410,
    GETDEFAULTSKINPATH=3420,
    GETAPPPATH=3430,
    IMGLDR_MAKEBMP=3500,
    OLDIMGLDR_MAKEBMP2=3510, // DEPRECATED, CUT
    IMGLDR_MAKEBMP2=3511,
    IMGLDR_REQUESTSKINBITMAP=3520,
    IMGLDR_RELEASESKINBITMAP=3530,
    REGISTERSKINFILTER=3600,
    UNREGISTERSKINFILTER=3610,
    REAPPLYSKINFILTERS=3620,
    FILTERSKINCOLOR=3630,
    GETNUMCOLORSETS=3650,
    ENUMCOLORSET=3660,
    GETNUMCOLORGROUPS=3670,
    ENUMCOLORGROUP=3680,
    SETCOLORSET=3690,
    GETCOLORSET=3695,
    PAINTSET_RENDER=3700,
    PAINTSET_RENDERTITLE=3710,
    DRAWANIMATEDRECTS=3800,
    SENDDDECOMMAND=3900,
    XMLREADER_REGISTERCALLBACK=4000,
    XMLREADER_UNREGISTERCALLBACK=4010,
    XMLREADER_LOADFILE=4020,
    WNDTRACKADD=4100,
    WNDTRACKREMOVE=4110,
    WNDTRACKDOCK=4120,
    WNDTRACKDOCK2=4130,
    WNDTRACKSTARTCOOPERATIVE=4140,
    WNDTRACKENDCOOPERATIVE=4150,
    WNDTRACKWASCOOPERATIVE=4160,
    WNDTRACKINVALIDATEALL=4170,
    GETNUMCOMPONENTS=4200,
    GETCOMPONENTGUID=4210,
    GETCOMPONENTNAME=4220,
/*retired    GETCOMPONENTINFO=4230, */
    COMPON_GETCFGINTERFACE=4240,
    SKINWND_CREATEBYGUID=4250,
    SKINWND_CREATEBYGROUPID=4251,
    SKINWND_TOGGLEBYGUID=4252,
    SKINWND_TOGGLEBYGROUPID=4253,
    SKINWND_DESTROY=4254,
    SKINWND_GETNUMBYGUID=4255,
    SKINWND_ENUMBYGUID=4256,
    SKINWND_GETNUMBYGROUPID=4257,
    SKINWND_ENUMBYGROUPID=4258,
    SKINWND_ATTACHTOSKIN=4259,
    WNDHOLDER_REGISTER=4270,
    WNDHOLDER_UNREGISTER=4271,
/*retired    COMP_RENDERELEMENT=4300, */
    COMP_NOTIFYSCRIPTS=4310,
    SKIN_GETCONTAINER=4500,
    SKIN_GETLAYOUT=4510,
    ROOTWNDFROMPOINT=5000,
    REGISTERROOTWND=5100,
    UNREGISTERROOTWND=5200,
    ROOTWNDISVALID=5300,
    FORWARDONCHAR=5400,
    FORWARDONKEYDOWN=5410,
    FORWARDONKEYUP=5420,
    FORWARDONSYSKEYDOWN=5421,
    FORWARDONSYSKEYUP=5422,
    FORWARDONKILLFOCUS=5430,
    FILEOPEN=5500,
    FILECLOSE=5510,
    FILEREAD=5520,
    FILEWRITE=5530,
    FILESEEK=5540,
    FILETELL=5550,
    FILEGETFILESIZE=5560,
    FILEEXISTS=5570,
    FILEREMOVE=5580,
    FILEMOVE=5585,
    FONT_TEXTOUT=5600,
    FONT_GETINFO=5610,
    HINT_GARBAGECOLLECT=5700,
    CONSOLE_OUTPUTSTRING=5800,
    LOADSKINFILE=5901,
    UNLOADSKINPART=5902,
    NAVIGATEURL=6000,
    ___RETIREDMETHOD=6010,
    GETINTPUBLIC=6100,
    SETINTPUBLIC=6110,
    COREGETWRITEPOSITION=6200,
    PARSE=6301,
    SYSMALLOC=6400,
    SYSFREE=6410,
    SYSREALLOC=6420,
    VARMGRTRANSLATE = 6500,
    TIMERADD = 6600,
    TIMERREMOVE = 6610,
    MAIN_GETCOMMANDLINE = 6620,
	MAIN_SETWINDOWTITLE = 6630,
    CONFIG_REGISTERCFGITEM = 6700,
    CONFIG_DEREGISTERCFGITEM = 6710,
    CONFIG_GETNUMCFGITEMS = 6720,
    CONFIG_ENUMCFGITEM = 6730,
    CONFIG_GETCFGITEMBYGUID = 6740,

    POPUPEXIT_CHECK = 6800,
    POPUPEXIT_SIGNAL = 6810,
    POPUPEXIT_REGISTER= 6820,
    POPUPEXIT_DEREGISTER= 6830,

    PREFERENCES_REGISTERGROUP = 6900,
    PREFERENCES_GETNUMGROUPS = 6910,
    PREFERENCES_ENUMGROUP = 6920,

    // 7030 = COREREGISTEREXTENSION
    // 7040 = COREGETEXTENSIONFAMILY

    XUI_NEW = 7100,
    XUI_DELETE = 7110,

    APPNODEACTIVATE_PUSH = 7200,
    APPNODEACTIVATE_POP = 7210,
    APPNODEACTIVATE_CHECK = 7220,
    APPNODEACTIVATE_BYPASS = 7230,

    //
    END_OF_DISPATCH
  };
};

// these are a bunch of helper implementations of the above methods
#include "apihelp.h"

extern ComponentAPI *api;

#endif
