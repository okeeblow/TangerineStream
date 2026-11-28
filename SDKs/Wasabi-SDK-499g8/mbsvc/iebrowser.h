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
#ifndef __BROWSER_H
#define __BROWSER_H

class BrowserWnd;

#define BROWSER_PARENT OSWnd
#define IDC_SINKOBJ 0x9871 // arbitrary unique id
#define MB_TIMERID1 0x1927
#define MB_TIMERID2 0x1928

class String;
class PtrList;

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <exdispid.h>

class BrowserSinkObj : public IDispEventImpl<IDC_SINKOBJ, BrowserSinkObj> {

      public:
        BEGIN_SINK_MAP(BrowserSinkObj)
          SINK_ENTRY(IDC_SINKOBJ, DISPID_BEFORENAVIGATE2, OnBeforeNavigate2)
          SINK_ENTRY(IDC_SINKOBJ, DISPID_DOCUMENTCOMPLETE, OnDocumentComplete)
        END_SINK_MAP()

          virtual void event_onBeforeNavigate(const char *url, int flags, const char *targetframename, int *cancel) { }
          virtual void event_onDocumentComplete(const char *url) { }

          HRESULT __stdcall OnDocumentComplete(IDispatch *pDisp, VARIANT *url);
          HRESULT __stdcall OnBeforeNavigate2(LPDISPATCH ppDisp, VARIANT* url, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers, BOOL *Cancel);
};

#include "../common/oswnd.h"
#include "../common/minibrowser.h"

struct IHTMLDocument2;
struct IWebBrowser2;

class BrowserWnd : public BROWSER_PARENT, public BrowserSinkObj, public MiniBrowserI {
public:
  BrowserWnd();
  virtual ~BrowserWnd();

  // RootWnd
  virtual int onInit();
  virtual void onSetVisible(int show);
  virtual int handleDesktopAlpha() { return 0; }

  // OSWnd
  virtual HWND getOSHandle();

  // MiniBrowser
  virtual int minibrowser_navigateUrl(const char *url);
  virtual void minibrowser_setHome(const char *url) { homepage = url; }
  virtual int minibrowser_back();
  virtual int minibrowser_forward();
  virtual int minibrowser_home();
  virtual int minibrowser_refresh();
  virtual int minibrowser_stop();
  virtual void minibrowser_setTargetName(const char *name);
  const char *minibrowser_getTargetName();
  const char *minibrowser_getCurrentUrl();
  virtual void minibrowser_addCB(MiniBrowserCallback *cb) { callbacks.addItem(cb); }
  virtual RootWnd *minibrowser_getRootWnd() { return this; }

  virtual void minibrowser_setScrollbarsFlag(int a); //BROWSER_SCROLLBARS_ALWAYS, BROWSER_SCROLLBARS_AUTO, BROWSER_SCROLLBARS_NEVER

  //
  virtual void timerCallback(int id);
  void onTargetNameTimer();

protected:
  virtual void event_onBeforeNavigate(const char *url, int flags, const char *targetframename, int *cancelnavigation);
  virtual void event_onDocumentComplete(const char *url);
  virtual int initBrowserStuff();
  virtual void freeBrowserStuff();
  virtual void onScrollbarsFlagTimer();

private:
  virtual int doSetTargetName(const char *name);
  virtual int doSetScrollbars();

  virtual void updateTargetName();
  virtual void updateScrollbars();

  IHTMLDocument2 *id;
  IWebBrowser2 *iw;
  IAxWinHostWindow *ih;
  IUnknown *iu,*contain;
  BOOL oleOk, atlInitialized;
  String homepage;
  String deferednavigate;
  String targetname;
  String curpage;
  int timerid;
  int timerset1;
  int timerset2;
  PtrList<MiniBrowserCallback> callbacks;
  int scrollbarsflag;
};

#endif

