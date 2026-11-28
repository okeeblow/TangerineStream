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
#ifndef __CONOBJ_H
#define __CONOBJ_H

#include <common/script/objcontroller.h>
#include <common/script/rootobj.h>
#include <core/sequence.h>
#include <core/corecb.h>

#ifdef GEN_FF
#ifndef FAKE_SCRIPTCORE
#define FAKE_SCRIPTCORE
#endif
#endif

// {2825A91B-D488-4245-AAF1-7059CF88437B}
static const GUID CORE_SCRIPTOBJECT_GUID = 
{ 0x2825a91b, 0xd488, 0x4245, { 0xaa, 0xf1, 0x70, 0x59, 0xcf, 0x88, 0x43, 0x7b } };

extern ScriptObjectController *coreController;
class PlayItem;
class CoreAdminScriptObjectController;
class svc_coreAdmin;

// -----------------------------------------------------------------------------------------------------
class ScriptCoreObject : public RootObjectInstance
#ifndef FAKE_SCRIPTCORE
, public ListSequencer, public CoreCallbackI/*, public CoreCallbackI*/ 
#endif
{
  friend class CoreAdminScriptObjectController;
  public:

    ScriptCoreObject(CoreToken token = 0x80000000);
    virtual ~ScriptCoreObject();

    void playFile(const char *file);
    void stop();
    void setVolume(int v);
    int getStatus();

/*    virtual int corecb_onStopped();
    virtual int corecb_onEndOfDecode();*/

    static scriptVar maki_playFile(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar file);
    static scriptVar maki_stop(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
//    static scriptVar maki_onStop(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
//    static scriptVar maki_setVolume(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar v);
//    static scriptVar maki_getStatus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);

    // Callback Methods in Maki.
    static scriptVar /*int*/ maki_onStarted(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_onStopped(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_onPaused(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_onUnpaused(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_onSeeked(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ newpos);
    static scriptVar /*int*/ maki_onVolumeChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ newvol);
    static scriptVar /*int*/ maki_onPanChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ newpan);
    static scriptVar /*int*/ maki_onEQStatusChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ newval);
    static scriptVar /*int*/ maki_onEQPreampChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ newval);
    static scriptVar /*int*/ maki_onEQBandChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ band, scriptVar /*int*/ newval);
    static scriptVar /*int*/ maki_onEQAutoChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ newval);
    static scriptVar /*int*/ maki_onStatusMsg(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ text);
    static scriptVar /*int*/ maki_onWarningMsg(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ text);
    static scriptVar /*int*/ maki_onErrorMsg(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ text);
    static scriptVar /*int*/ maki_onTitleChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ title);
    static scriptVar /*int*/ maki_onTitle2Change(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ title2);
    static scriptVar /*int*/ maki_onInfoChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ info);
    static scriptVar /*int*/ maki_onUrlChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ url);
    static scriptVar /*int*/ maki_onLengthChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ newlength);
    static scriptVar /*int*/ maki_onNextFile(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_onNeedNextFile(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ fileid);
    static scriptVar /*int*/ maki_onSetNextFile(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ playstring);
    static scriptVar /*int*/ maki_onErrorOccured(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ severity, scriptVar /*String*/ text);
    static scriptVar /*int*/ maki_onAbortCurrentSong(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_onEndOfDecode(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_onFileComplete(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ playstring);
    static scriptVar /*int*/ maki_onConvertersChainRebuilt(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_onMediaFamilyChange(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ newfamily);

    static scriptVar /*int*/ maki_setNextFile(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ playstr);
    static scriptVar /*int*/ maki_getStatus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*String*/ maki_getCurrent(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_getCurPlaybackNumber(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_getNumTracks(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_getPosition(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_getWritePosition(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_setPosition(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ ms);
    static scriptVar /*int*/ maki_getLength(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_getVolume(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*void*/ maki_setVolume(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ vol);
    static scriptVar /*int*/ maki_getPan(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*void*/ maki_setPan(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ bal);
    static scriptVar /*void*/ maki_setMute(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ mute);
    static scriptVar /*int*/ maki_getMute(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_getLeftVuMeter(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*int*/ maki_getRightVuMeter(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*void*/ maki_userButton(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ button);
    static scriptVar /*int*/ maki_getEqStatus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*void*/ maki_setEqStatus(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ enable);
    static scriptVar /*int*/ maki_getEqPreamp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*void*/ maki_setEqPreamp(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ pre);
    static scriptVar /*int*/ maki_getEqBand(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ band);
    static scriptVar /*void*/ maki_setEqBand(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ band, scriptVar /*int*/ val);
    static scriptVar /*int*/ maki_getEqAuto(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*void*/ maki_setEqAuto(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ enable);
    static scriptVar /*void*/ maki_setCustomMsg(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*String*/ text);
    static scriptVar /*void*/ maki_setPriority(SCRIPT_FUNCTION_PARAMS, ScriptObject *o, scriptVar /*int*/ priority);
    static scriptVar /*int*/ maki_getPriority(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);
    static scriptVar /*void*/ maki_rebuildConvertersChain(SCRIPT_FUNCTION_PARAMS, ScriptObject *o);



    static PtrList<ScriptCoreObject> objects;

  protected:
#ifndef FAKE_SCRIPTCORE
    virtual int getNumEntries() { return 1; }
    virtual const char *enumItem(int pos);
    virtual int getCurrent() { return 0; }
    virtual int setCurrent(int cur) { return 0; }
#endif

    // Callback Methods forwarded to Maki.
    virtual int corecb_onStarted();
    virtual int corecb_onStopped();
    virtual int corecb_onPaused();
    virtual int corecb_onUnpaused();
    virtual int corecb_onSeeked(int newpos);
    virtual int corecb_onVolumeChange(int newvol);
    virtual int corecb_onPanChange(int newpan);
    virtual int corecb_onEQStatusChange(int newval);
    virtual int corecb_onEQPreampChange(int newval);
    virtual int corecb_onEQBandChange(int band, int newval);
    virtual int corecb_onEQAutoChange(int newval);
    virtual int corecb_onStatusMsg(const char *text);
    virtual int corecb_onWarningMsg(const char *text);
    virtual int corecb_onErrorMsg(const char *text);
    virtual int corecb_onTitleChange(const char *title);
    virtual int corecb_onTitle2Change(const char *title2);
    virtual int corecb_onInfoChange(const char *info);
    virtual int corecb_onUrlChange(const char *url);
    virtual int corecb_onLengthChange(int newlength);
    virtual int corecb_onNextFile();
    virtual int corecb_onNeedNextFile(int fileid);
    virtual int corecb_onSetNextFile(const char *playstring);
    virtual int corecb_onErrorOccured(int severity, const char *text);
    virtual int corecb_onAbortCurrentSong();
    virtual int corecb_onEndOfDecode();
    virtual int corecb_onFileComplete(const char *playstring);
    virtual int corecb_onConvertersChainRebuilt();
    virtual int corecb_onMediaFamilyChange(const char *newfamily);


  private:
    // Only called by us and our friend the admin.
#ifndef FAKE_SCRIPTCORE
    virtual void initAsCreated();

    svc_coreAdmin *svc;
    CoreToken core_handle;
    String filetoplay;
    int curvol;
    int registered_sequencer;
#endif
};

// -----------------------------------------------------------------------------------------------------
class CoreScriptObjectController : public ScriptObjectControllerI {
  friend CoreAdminScriptObjectController;
  public:
    virtual const char *getClassName() { return "Core"; } 
    virtual const char *getAncestorClassName() { return "Object"; }  
    virtual ScriptObjectController *getAncestorController() { return WASABI_API_MAKI->maki_getController(rootObjectGuid); } 
    virtual int getNumFunctions(); 
    virtual const function_descriptor_struct *getExportedFunctions() { return exportedFunction; } 
    virtual GUID getClassGuid() { return CORE_SCRIPTOBJECT_GUID; } 
    virtual int getInstantiable() { return 1; } 
    virtual int getReferenceable() { return 1; } 
    virtual ScriptObject *instantiate(); 
    virtual void *encapsulate(ScriptObject *o); 
    virtual void destroy(ScriptObject *o); 
    virtual void deencapsulate(void *o);
    
  private:
    static function_descriptor_struct exportedFunction[];

    // Only called by us and our friend the admin.
    virtual ScriptObject *instantiate(CoreToken token = 0x80000000);
};

// -----------------------------------------------------------------------

#endif
