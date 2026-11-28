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
#include "iebrowser.h"
#include "mbsvc.h"
#include <atlimpl.cpp>

#include <studio/api.h>
#include <bfc/attribs/cfgitem.h>

#include <windows.h>
#include <mshtml.h>
#include <atldef.h>
#include <atliface.h>
#include <exdisp.h>

extern "C" BOOL AtlMain(HINSTANCE hInstance);
extern "C" void AtlQuit(void);

STDAPI WriteBSTR(BSTR *pstrDest, LPCWSTR szSrc) {
  *pstrDest = SysAllocString( szSrc );
  if( !(*pstrDest) ) return E_OUTOFMEMORY;
  return NOERROR;
}

STDAPI FreeBSTR(BSTR* pstr) {
  if( *pstr == NULL ) return S_FALSE;
  SysFreeString( *pstr );
  return NOERROR;
}

HRESULT writeBString(BSTR* psz, const char *str) {
  WCHAR WideStr[WA_MAX_PATH];
  String s = str;
  if (s.isempty()) s = "";
  MultiByteToWideChar(CP_ACP, 0, s, -1, WideStr, WA_MAX_PATH);
  return WriteBSTR(psz, WideStr);
}

BrowserWnd::BrowserWnd() {
  setVirtual(0);
	iw = NULL;
	ih = NULL;
	iu = NULL;
	contain = NULL;
	oleOk = FALSE;
  homepage = "about:blank";
  atlInitialized = FALSE;
  timerset1 = 0;
  timerset2 = 0;
  scrollbarsflag = BROWSER_SCROLLBARS_DEFAULT;
}

BrowserWnd::~BrowserWnd() {
  if (timerset1) {
    killTimer(MB_TIMERID1);
    timerset1 = 0;
  }
  if (timerset2) {
    killTimer(MB_TIMERID2);
    timerset2 = 0;
  }
  freeBrowserStuff();
}

int BrowserWnd::onInit() {
  BROWSER_PARENT::onInit();
  if (isVisible())
    onSetVisible(1);
  updateScrollbars();
  return 1;
}

void BrowserWnd::onSetVisible(int show) {
  if (show) initBrowserStuff();
}

int BrowserWnd::initBrowserStuff() {
  if (iw) return 1;
  if(!atlInitialized) {
#ifdef WASABINOMAINAPI
    AtlMain(WASABI_API_APP->main_gethInstance());
#else
    AtlMain(wacmb->gethInstance());
#endif
    atlInitialized=TRUE;
  }

  if (SUCCEEDED(OleInitialize(NULL))) 
    oleOk = TRUE;
  if (!oleOk) return 1;

// {280876CF-48C0-40bc-8E86-73CE6BB462E5}
const GUID options_guid = 
{ 0x280876cf, 0x48c0, 0x40bc, { 0x8e, 0x86, 0x73, 0xce, 0x6b, 0xb4, 0x62, 0xe5 } };
	HWND hwndControl=gethWnd();

  int usemozilla = 0;

  #ifdef WASABI_COMPILE_CONFIG
  usemozilla = _intVal(WASABI_API_CONFIG->config_getCfgItemByGuid(options_guid), "Use Mozilla instead of IE for minibrowser");
  #endif

	if (SUCCEEDED(ATL::AtlAxCreateControlEx(
    usemozilla ? L"{1339B54C-3453-11D2-93B9-000000000000}" : L"Shell.Explorer",
		hwndControl,NULL,&contain,&iu,IID_IWebBrowser2,NULL)) && iu && contain) {
		iu->QueryInterface(IID_IWebBrowser2,(void**)&iw);

		if (iw) {

      iw->put_RegisterAsBrowser(TRUE);

      AtlGetObjectSourceInterface(iw, &m_libid, &m_iid, &m_wMajorVerNum, &m_wMinorVerNum);
      DispEventAdvise(iw, &m_iid);

      if (deferednavigate.isempty()) {
        if(!homepage.isempty())
          minibrowser_navigateUrl(homepage);
        else
          minibrowser_navigateUrl("about:blank");
      } else 
        minibrowser_navigateUrl(deferednavigate);

		  ih=NULL;
		  contain->QueryInterface(IID_IAxWinHostWindow,(void**)&ih);

		  if (ih) {
		  }

    }
	} 
	ShowWindow(hwndControl,SW_SHOWNA);
  return 1;
}

void BrowserWnd::freeBrowserStuff() {
  if (oleOk) {
    if (ih) {
  	  ih->Release();
  	  ih=NULL;
    }
    if (iu) {
  	  iu->Release();
  	  iu=NULL;
    }
    if (contain) {
  	  contain->Release();
  	  contain=NULL;
    }
    if (iw) {
      DispEventUnadvise(iw, &m_iid);
  	  iw->Release();
  	  iw=NULL;
    }
    if (atlInitialized) {
      AtlQuit();
      atlInitialized = FALSE;
    }
    OleUninitialize();
    oleOk = FALSE;
#ifndef WASABINOMAINAPI
    api->hint_garbageCollect();
#endif
  }
}

int BrowserWnd::minibrowser_navigateUrl(const char *url) {
  curpage = url;
  if (!iw) {
    deferednavigate = url;
    return 0;
  }
  wchar_t wcstr[WA_MAX_PATH];
  mbstowcs(wcstr, url, sizeof(wcstr));

  iw->Navigate(wcstr, NULL, NULL, NULL, NULL);
  return 1;
}

int BrowserWnd::minibrowser_back() {
  ASSERT(iw != NULL);
  iw->GoBack();
  return 1;
}

int BrowserWnd::minibrowser_forward() {
  ASSERT(iw != NULL);
  iw->GoForward();
  return 1;
}

int BrowserWnd::minibrowser_refresh() {
  ASSERT(iw != NULL);
  iw->Refresh();
  return 1;
}

int BrowserWnd::minibrowser_home() {
  ASSERT(iw != NULL);
  minibrowser_navigateUrl(homepage);
  return 1;
}

int BrowserWnd::minibrowser_stop() {
  ASSERT(iw != NULL);
  iw->Stop();
  return 1;
}

HWND BrowserWnd::getOSHandle() {
  return GetWindow(gethWnd(), GW_CHILD); // assumes setVirtual(0) in constructor
}

void BrowserWnd::onTargetNameTimer() {
  updateTargetName();
}

void BrowserWnd::onScrollbarsFlagTimer() {
  updateScrollbars();
}

void BrowserWnd::timerCallback(int id) {
  switch (id) {
    case MB_TIMERID1:
      onTargetNameTimer();
      return;
    case MB_TIMERID2: 
      onScrollbarsFlagTimer();
      return;
  }
  BROWSER_PARENT::timerCallback(id);
}

void BrowserWnd::minibrowser_setTargetName(const char *name) {
  targetname = name;
  updateTargetName();
}

void BrowserWnd::updateTargetName() {
  if (!doSetTargetName(targetname)) {
    if (!timerset1) { setTimer(MB_TIMERID1, 100); timerset1 = 1; }
    return;
  } else {
    if (timerset1) { killTimer(MB_TIMERID1); timerset1 = 0; }
  }
}

int BrowserWnd::doSetTargetName(const char *name) {
  if (!iw) return 0;
  IDispatch *id;
  if (SUCCEEDED(iw->get_Document(&id)) && id) {
    IHTMLDocument2 *doc;
    if (SUCCEEDED(id->QueryInterface(IID_IHTMLDocument2, (void **)&doc)) && doc) {
      IHTMLWindow2 *w;
      if (SUCCEEDED(doc->get_parentWindow(&w)) && w) {
        
        wchar_t wcstr[WA_MAX_PATH];
        mbstowcs(wcstr, targetname, sizeof(wcstr));
        w->put_name(wcstr);
        w->Release();
        doc->Release();
        id->Release();
        return 1;
      }
    doc->Release();
    }
    id->Release();
  }
  return 0;
}

const char *BrowserWnd::minibrowser_getTargetName() {
  return targetname;
}

void BrowserWnd::event_onBeforeNavigate(const char *url, int flags, const char *targetframename, int *cancelnavigation) {
  int i=0;
  foreach(callbacks)
    int r = callbacks.getfor()->minibrowsercb_onBeforeNavigate(url, flags, targetframename);
    if (i++ == 0) *cancelnavigation = r;
  endfor;
  updateScrollbars();
}

void BrowserWnd::minibrowser_setScrollbarsFlag(int a) {
  scrollbarsflag = a;
  updateScrollbars();
}

void BrowserWnd::updateScrollbars() {
  if (!doSetScrollbars()) {
    if (!timerset2) { setTimer(MB_TIMERID2, 100); timerset2 = 1; }
    return;
  } else {
    if (timerset2) { killTimer(MB_TIMERID2); timerset2 = 0; }
  }
}

void BrowserWnd::event_onDocumentComplete(const char *url) {
  if (!targetname.isempty())
    minibrowser_setTargetName(targetname);
  foreach(callbacks)
    callbacks.getfor()->minibrowsercb_onDocumentComplete(url);
  endfor;
  updateScrollbars();
}

int BrowserWnd::doSetScrollbars() {
  if (!iw) return 0;
  if (scrollbarsflag == BROWSER_SCROLLBARS_DEFAULT) return 1;
  IDispatch *id;
  if (SUCCEEDED(iw->get_Document(&id)) && id) {
    IHTMLDocument2 *doc;
    if (SUCCEEDED(id->QueryInterface(IID_IHTMLDocument2, (void **)&doc)) && doc) {
      IHTMLElement *e;
      if (SUCCEEDED(doc->get_body(&e))) {
        IHTMLStyle *s;
        if (SUCCEEDED(e->get_style(&s))) {
          BSTR a;
          switch (scrollbarsflag) {
            case BROWSER_SCROLLBARS_ALWAYS:
              writeBString(&a, "scroll");
              break;
            case BROWSER_SCROLLBARS_AUTO:
              writeBString(&a, "auto");
              break;
            case BROWSER_SCROLLBARS_NEVER:
              writeBString(&a, "hidden");
              break;
          }
          s->put_overflow(a);
          FreeBSTR(&a);
          s->Release();
          return 1;
        }
        e->Release();
      }
      doc->Release();
    }
    id->Release();
  }
  return 0;
}

const char *BrowserWnd::minibrowser_getCurrentUrl() {
  return curpage;
}

HRESULT BrowserSinkObj::OnBeforeNavigate2(LPDISPATCH pDisp, VARIANT* _url, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers, BOOL *Cancel) {

  char url[WA_MAX_PATH]="";
  wcstombs(url, _url->bstrVal, WA_MAX_PATH-1);
  url[WA_MAX_PATH-1]=0;

  int flags = Flags->intVal;

  char target[256]="";
  if (TargetFrameName->bstrVal != NULL) {
    wcstombs(target, TargetFrameName->bstrVal, 255);
    target[255]=0;
  }

  int cancel=0;
  event_onBeforeNavigate(url, flags, target, &cancel);

  *Cancel = cancel ? -1 : 0;
  return S_OK;
}

HRESULT BrowserSinkObj::OnDocumentComplete(IDispatch *pDisp, VARIANT *_url) {
  char url[WA_MAX_PATH]="";
  wcstombs(url, _url->bstrVal, WA_MAX_PATH-1);
  url[WA_MAX_PATH-1]=0;
  event_onDocumentComplete(url);
  return S_OK;
}

