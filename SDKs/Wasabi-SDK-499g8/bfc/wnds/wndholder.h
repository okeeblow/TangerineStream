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
#ifndef __WINDOWHOLDER_H
#define __WINDOWHOLDER_H

#include "../../common/guiobjwnd.h"
#include "../ptrlist.h"
#include "../../common/wndcb.h"
#include "../../bfc/timerclient.h"

class svc_windowCreate;

#define WINDOWHOLDER_PARENT GuiObjectWnd 


/**
  Class 

  @short 
  @author Nullsoft
  @ver 1.0
  @see 
*/
class COMEXP WindowHolder : public Dispatchable {
  public:
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    RootWnd *onInsertWindow(GUID g, const char *groupid);
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void onRemoveWindow(int deferred=0);

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    int wantGuid(GUID g);
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    int wantGroup(const char *groupid);

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    GUID getCurGuid();
    const char *getCurGroupId();
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    RootWnd *getCurRootWnd();
    const char *getCurId();

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    RootWnd *getRootWndPtr();

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    int acceptsGenericGuid();
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    int acceptsGenericGroup();

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    int wndholder_getPreferences(int what);
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void wndholder_onNeedReloadGroup(const char *id);

    void cancelDeferredRemove();
    int wndholder_wantAutoFocus();
    int wndholder_isAutoAvailable();

  enum {
    WNDHOLDER_ONINSERTWINDOW=50,
    WNDHOLDER_ONREMOVEWINDOW=100,
    WNDHOLDER_WANTGUID=150,
    WNDHOLDER_WANTGROUP=200,
    WNDHOLDER_GETROOTWNDPTR=250,
    WNDHOLDER_GETCURGUID=300,
    WNDHOLDER_GETCURGROUPID=350,
    WNDHOLDER_GETCURROOTWND=400,
    WNDHOLDER_GETCURID=450,
    WNDHOLDER_ISGENERICGUID=500,
    WNDHOLDER_ISGENERICGROUP=550,
    WNDHOLDER_GETPREFERENCES=600,
    WNDHOLDER_ONNEEDRELOADGRP=650,
    WNDHOLDER_CANCELDEFERREDREMOVE=660,
    WNDHOLDER_WANTAUTOFOCUS=670,
    WNDHOLDER_ISAUTOAVAILABLE=680,
  };
};

inline RootWnd *WindowHolder::onInsertWindow(GUID g, const char *groupid) {  
  return _call(WNDHOLDER_ONINSERTWINDOW, (RootWnd *)NULL, g, groupid);
}

inline void WindowHolder::onRemoveWindow(int def) {  
  _voidcall(WNDHOLDER_ONREMOVEWINDOW, def);
}

inline int WindowHolder::wantGuid(GUID g) {  
  return _call(WNDHOLDER_WANTGUID, 0, g);
}

inline int WindowHolder::wantGroup(const char *groupid) {  
  return _call(WNDHOLDER_WANTGROUP, 0, groupid);
}                                             

inline RootWnd *WindowHolder::getRootWndPtr() {
  return _call(WNDHOLDER_GETROOTWNDPTR, (RootWnd *)NULL);
}

inline GUID WindowHolder::getCurGuid() {
  return _call(WNDHOLDER_GETCURGUID, INVALID_GUID);
}

inline const char *WindowHolder::getCurGroupId() {
  return _call(WNDHOLDER_GETCURGROUPID, (const char *)NULL);
}

inline RootWnd *WindowHolder::getCurRootWnd() {
  return _call(WNDHOLDER_GETCURROOTWND, (RootWnd *)NULL);
}

inline const char *WindowHolder::getCurId() {
  return _call(WNDHOLDER_GETCURID, (const char *)NULL);
}

inline int WindowHolder::acceptsGenericGuid() {
  return _call(WNDHOLDER_ISGENERICGUID, 0);
}

inline int WindowHolder::acceptsGenericGroup() {
  return _call(WNDHOLDER_ISGENERICGROUP, 0);
}

inline int WindowHolder::wndholder_getPreferences(int what) {
  return _call(WNDHOLDER_GETPREFERENCES, 0, what);
}

inline void WindowHolder::wndholder_onNeedReloadGroup(const char *id) {
  _voidcall(WNDHOLDER_ONNEEDRELOADGRP, id);
}

inline void WindowHolder::cancelDeferredRemove() {
  _voidcall(WNDHOLDER_CANCELDEFERREDREMOVE);
}

inline int WindowHolder::wndholder_wantAutoFocus() {
  return _call(WNDHOLDER_WANTAUTOFOCUS, 1);
}

inline int WindowHolder::wndholder_isAutoAvailable() {
  return _call(WNDHOLDER_ISAUTOAVAILABLE, 1);
}

class DeferredRemove;

/**
  Class 

  @short 
  @author Nullsoft
  @ver 1.0
  @see 
*/
class COMEXP WindowHolderI : public WindowHolder {
  public:

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    WindowHolderI();
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual ~WindowHolderI();

    virtual RootWnd *onInsertWindow(GUID g, const char *groupid);
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual void onRemoveWindow(int deferred);

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void addAcceptGuid(GUID g);
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void addAcceptGroup(const char *groupid);

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void setAcceptAllGuids(int tf);
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void setAcceptAllGroups(int tf);

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int acceptsGenericGroup() { return generic_group; }
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int acceptsGenericGuid() { return generic_guid; }

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int wantGuid(GUID g);
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int wantGroup(const char *groupid);

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual void onInsert(RootWnd *w, const char *id) {};
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual void onRemove(RootWnd *w, const char *id) {};

    virtual RootWnd *getRootWndPtr()=0; 
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual GUID getCurGuid() { return cur_guid; }
    virtual const char *getCurGroupId() { return cur_groupid; }

    virtual RootWnd *getCurRootWnd() { return wnd; }

    virtual GUID *getFirstAcceptedGuid();
    virtual const char *getFirstAcceptedGroup();
    virtual const char *getCurId() { return cur_id; }

    virtual void cancelDeferredRemove();
    virtual int wndholder_isAutoAvailable() { return 1; }
    
    GUID getDeferedGuid() { return defered_guid; }

    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int wndholder_getPreferences(int what)=0;
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual void wndholder_onNeedReloadGroup(const char *id);

  private:

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    RootWnd *createWindow(const GUID *g, const char *groupid);
    virtual int wndholder_wantAutoFocus();
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void destroyWindow();

    RootWnd *wnd;
    GUID cur_guid;
    String cur_groupid;
    String cur_id;
    PtrList<GUID> accepted_guids;
    PtrList<String> accepted_groups;
    int generic_guid;
    int generic_group;

    svc_windowCreate *wc_svc;
    GUID defered_guid;

    DeferredRemove *dr;

  protected:

    RECVS_DISPATCH;
};


/**
  Class 

  @short 
  @author Nullsoft
  @ver 1.0
  @see 
*/
class COMEXP WindowHolderWnd : public WINDOWHOLDER_PARENT, public WindowHolderI {

  public:

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    WindowHolderWnd();
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual ~WindowHolderWnd();

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int onInit();

    virtual RootWnd *getRootWndPtr() { return this; }

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual void onInsert(RootWnd *w, const char *id);
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual void onRemove(RootWnd *w, const char *id);

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int onResize();

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int handleRatio();
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int handleDesktopAlpha();
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int handleTransparency();

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int wndholder_getPreferences(int what) { return getPreferences(what); }
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int getPreferences(int what);

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void setAutoOpen(int tf) { autoopen = tf; }
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void setAutoClose(int tf) { autoclose = tf; }
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void setNoCmdBar(int tf) { nocmdbar = tf; if (isInited()) invalidate(); }
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void setNoAnim(int tf) { noanim = tf; }

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    virtual int onGroupChange(const char *grpid);
    virtual int wndholder_wantAutoFocus();
    void setAutoFocus(int autof) { autofocus = autof; }
    void setAutoAvailable(int autoa) { autoavail = autoa; }
    virtual int wndholder_isAutoAvailable() { return autoavail; }

  private:

    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void notifyOnRemove(); // no virtual please
    
    /**
      Method
    
      @see 
      @ret 
      @param 
    */
    void notifyOnInsert(); // no virtual please
    virtual void onSetVisible(int show);
    virtual int onDeferredCallback(int p1, int p2);

    int autoopen;
    int autoclose;
    int nocmdbar;
    int noanim;
    int has_wnd;
    int autofocus;
    int autoavail;
};

class DeferredRemove : public TimerClientDI {
  public:
    DeferredRemove(WindowHolderI *parent) : whi(parent) {}
    virtual ~DeferredRemove() {}

    void post() {
      timerclient_postDeferredCallback(1, 0);
    }

    virtual int timerclient_onDeferredCallback(int p1, int p2) {
      if (p1 == 1 && whi != NULL) whi->onRemoveWindow(0);
      else return TimerClientDI::timerclient_onDeferredCallback(p1, p2);
      return 0;
    }

  private:
    WindowHolderI *whi;
};

#endif
