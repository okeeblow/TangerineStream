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
#ifndef __SVC_ROLESERVER_H
#define __SVC_ROLESERVER_H

#include "../../bfc/dispatch.h"
#include "../../bfc/string.h"
#include "../../bfc/ptrlist.h"
#include "services.h"
#include "../../common/script/scriptobj.h"

class RootWnd;

#define FLATTENFLAG_FLATTEN    1
#define FLATTENFLAG_UNFLATTEN -1
#define FLATTENFLAG_ASKPARENT 0

class NOVTABLE roleServerObject : public Dispatchable {
  public:
    int wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HWND gethWnd();
    int flattenContent(HWND *w);

    enum {
      RSO_WNDPROC=0,
      RSO_GETHWND=10,
      RSO_FLATTENCONTENT=20,
    };
};

inline int roleServerObject::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return _call(RSO_WNDPROC, 0, hWnd, uMsg, wParam, lParam);
}

inline HWND roleServerObject::gethWnd() {
  return _call(RSO_GETHWND, (HWND)NULL);
}

inline int roleServerObject::flattenContent(HWND *w) {
  return _call(RSO_FLATTENCONTENT, 0, w);
}

class roleServerObjectI : public roleServerObject {
  public:
    
    roleServerObjectI(HWND parent, RootWnd *w);
    virtual ~roleServerObjectI();

    virtual int wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual HWND createWindow(HWND parent)=0;
    virtual int flattenContent(HWND *w);

  protected:

    ScriptObject *getScriptObject();
    virtual RootWnd *getWnd();
    virtual HWND gethWnd();
    WNDPROC getOldProc();

    HWND hwnd, parent;
    RootWnd *wnd;
    long (__stdcall *oldproc)(struct HWND__ *,unsigned int,unsigned int,long);
    int triedyet;

    RECVS_DISPATCH;
};

class NOVTABLE svc_accRoleServer : public Dispatchable {
public:
  static FOURCC getServiceType() { return WaSvc::ACCESSIBILITYROLESERVER; }
  
  int handleRole(int role);
  roleServerObject *createObject(HWND parent, RootWnd *attached_wnd);
  void destroyObject(roleServerObject *obj);

  enum {
    RS_HANDLEROLE=10,
    RS_CREATEOBJECT=20,
    RS_DESTROYOBJECT=30
  };

};

inline int svc_accRoleServer::handleRole(int role) {
  return _call(RS_HANDLEROLE, 0, role);
}

inline roleServerObject *svc_accRoleServer::createObject(HWND parent, RootWnd *attached_wnd) {
  return _call(RS_CREATEOBJECT, (roleServerObject *)NULL, parent, attached_wnd);
}

inline void svc_accRoleServer::destroyObject(roleServerObject *obj) { 
  _voidcall(RS_DESTROYOBJECT, obj);
}



class svc_accRoleServerI : public svc_accRoleServer {
  
  public:

    virtual int handleRole(int role)=0;
    virtual roleServerObject *createObject(HWND parent, RootWnd *attached_wnd)=0;
    virtual void destroyObject(roleServerObject *obj)=0;

  protected:
    RECVS_DISPATCH;
};

#include "servicei.h"
template <class T>
class AccRoleServerCreatorSingle : public waServiceFactoryTSingle<svc_accRoleServer, T> {
public:
  svc_accRoleServer *getHandler() {
    return getSingleService();
  }
};

#include "../../bfc/svc_enum.h"
#include "../../bfc/string.h"

class AccRoleServerEnum : public SvcEnumT<svc_accRoleServer> {
public:
  AccRoleServerEnum(int role) : roletest(role) { }
protected:
  virtual int testService(svc_accRoleServer *svc) {
    return (svc->handleRole(roletest));
  }
private:
  int roletest;
};


#endif
