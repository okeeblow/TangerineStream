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
#ifndef __APIWNDMGR_H
#define __APIWNDMGR_H

#include "../../bfc/api/api_wndmgr.h"
#include "../../bfc/stack.h"

class WndMgrApi : public wndmgr_apiI {
  public:
    WndMgrApi();
    virtual ~WndMgrApi();

    virtual void wndTrackAdd(RootWnd *wnd);
    virtual void wndTrackRemove(RootWnd *wnd);
    virtual BOOL wndTrackDock(RootWnd *wnd, RECT *r, int mask);
    virtual BOOL wndTrackDock2(RootWnd *wnd, RECT *r, RECT *orig_r, int mask);
    virtual void wndTrackStartCooperative(RootWnd *wnd);
    virtual void wndTrackEndCooperative();
    virtual int wndTrackWasCooperative();
    virtual void wndTrackInvalidateAll();
    virtual int skinwnd_toggleByGuid(GUID g, const char *prefered_container = NULL, int container_flag = 0, RECT *sourceanimrect = NULL, int transcient = 0);
    virtual int skinwnd_toggleByGroupId(const char *groupid, const char *prefered_container = NULL, int container_flag = 0, RECT *sourceanimrect = NULL, int transcient = 0);
    virtual RootWnd *skinwnd_createByGuid(GUID g, const char *prefered_container = NULL, int container_flag = 0, RECT *sourceanimrect = NULL, int transcient = 0, int starthidden = 0, int *isnew=NULL);
    virtual RootWnd *skinwnd_createByGroupId(const char *groupid, const char *prefered_container = NULL, int container_flag = 0, RECT *sourceanimrect = NULL, int transcient = 0, int starthidden = 0, int *isnew=NULL);
    virtual void skinwnd_destroy(RootWnd *w, RECT *destanimrect = NULL);
    virtual int skinwnd_getNumByGuid(GUID g);
    virtual RootWnd *skinwnd_enumByGuid(GUID g, int n);
    virtual int skinwnd_getNumByGroupId(const char *groupid);
    virtual RootWnd *skinwnd_enumByGroupId(const char *groupid, int n);
    virtual void skinwnd_attachToSkin(RootWnd *w, int side, int size);
    virtual ScriptObject *skin_getContainer(const char *container_name);
    virtual ScriptObject *skin_getLayout(ScriptObject *container, const char *layout_name);
    virtual void wndholder_register(WindowHolder *wh);
    virtual void wndholder_unregister(WindowHolder *wh);
    virtual int messageBox(const char *txt, const char *title, int flags, const char *not_anymore_identifier, RootWnd *parenwnt);
    virtual RootWnd *getModalWnd();
    virtual void pushModalWnd(RootWnd *w = MODALWND_NOWND);
    virtual void popModalWnd(RootWnd *w = MODALWND_NOWND);
    virtual void drawAnimatedRects(const RECT *r1, const RECT *r2);
    virtual int autopopup_registerGuid(GUID g, const char *desc, const char *prefered_container = NULL, int container_flag = 0);
    virtual int autopopup_registerGroupId(const char *groupid, const char *desc, const char *prefered_container = NULL, int container_flag = 0);
    virtual void autopopup_unregister(int id);
    virtual int autopopup_getNumGuids();
    virtual GUID autopopup_enumGuid(int n);
    virtual int autopopup_getNumGroups();
    virtual const char *autopopup_enumGroup(int n);
    virtual const char *varmgr_translate(const char *str);
    virtual Container *newDynamicContainer(const char *name, int transcient);
    virtual const char *autopopup_enumGuidDescription(int n);
    virtual const char *autopopup_enumGroupDescription(int n);

  private:

    static Stack<RootWnd*> modal_wnd_stack;
    String ret;
};


#endif // __APIWNDMGR_H
