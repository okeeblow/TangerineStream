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

#include "videownd.h"
#include "vid_overlay.h"

#include <bfc/canvas.h>
#include <bfc/attribs/attribs.h>
#include <mmsystem.h>

#include <multimon.h>

#define INIT_DIRECTDRAW_STRUCT(x) (ZERO(x), x.dwSize=sizeof(x))
#define OV_COL_R 16
#define OV_COL_G 0
#define OV_COL_B 16

extern _bool vid_vsync;

OverlayVideoOutput::OverlayVideoOutput() {
  lpDD=NULL;
  lpddsOverlay=NULL;
  lpddsPrimary=NULL;
  is_fullscreen=0;
  yuy2_output=uyvy_output=0;
  m_parent=NULL;
  initing=false;
  needchange=0;
  MEMSET(&m_oldrd,0,sizeof(m_oldrd));
  m_mon_x=m_mon_y=0;
  fullscreen_controls = NULL;
}

OverlayVideoOutput::~OverlayVideoOutput() {
  if(is_fullscreen) removeFullScreen();
  if(lpddsOverlay) {
    LPDIRECTDRAWSURFACE o=lpddsOverlay;
    lpddsOverlay=NULL;
    o->Release();
  }
  if(lpddsPrimary) lpddsPrimary->Release();
  if (lpDD) lpDD->Release();	// BU added NULL check in response to talkback
  delete fullscreen_controls;
}

static BOOL WINAPI DDEnumCallbackEx(GUID FAR *lpGUID, LPSTR     lpDriverDescription, LPSTR     lpDriverName, LPVOID    lpContext, HMONITOR  hm) {
  OverlayVideoOutput *ovo=(OverlayVideoOutput *)lpContext;
  if(ovo->m_found_devguid) return 1;
  if(hm==ovo->m_monitor_to_find) {
    ovo->m_devguid=*lpGUID;
    ovo->m_found_devguid=1;
  }
  return 1;
}

int OverlayVideoOutput::create(VideoWnd *parent, int w, int h, const char *ptype, int flipit, float aspectratio) {
  type=ptype;
  width=w;
  height=h;
  flip=flipit;
  m_parent=parent;

  initing=true;
  HWND hwnd=parent->gethWnd();

  if (lpDD) lpDD->Release();
  lpDD=NULL;
  m_mon_x=m_mon_y=0;

  //find the correct monitor if multiple monitor support is present
  m_found_devguid=0;
  {
	  HINSTANCE h=LoadLibrary("user32.dll");
	  if (h) {
			HMONITOR (WINAPI *Mfp)(POINT pt, DWORD dwFlags) = (HMONITOR (WINAPI *)(POINT,DWORD)) GetProcAddress(h,"MonitorFromPoint");
      HMONITOR (WINAPI *Mfr)(LPCRECT lpcr, DWORD dwFlags) = (HMONITOR (WINAPI *)(LPCRECT, DWORD)) GetProcAddress(h, "MonitorFromRect");
      HMONITOR (WINAPI *Mfw)(HWND wnd, DWORD dwFlags)=(HMONITOR (WINAPI *)(HWND, DWORD)) GetProcAddress(h, "MonitorFromWindow");
      BOOL (WINAPI *Gmi)(HMONITOR mon, LPMONITORINFO lpmi) = (BOOL (WINAPI *)(HMONITOR,LPMONITORINFO)) GetProcAddress(h,"GetMonitorInfoA");    
			if (Mfp && Mfr && Mfw && Gmi) {
        RECT r;
        GetWindowRect(hwnd,&r);
        HMONITOR hm=Mfr(&r,NULL);
        if(hm) {
          HINSTANCE hdd = LoadLibrary("ddraw.dll");
          if(hdd) {
            typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKEXA)(GUID FAR *, LPSTR, LPSTR, LPVOID, HMONITOR);
            typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEX)( LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);
            LPDIRECTDRAWENUMERATEEX lpDDEnumEx;
            lpDDEnumEx = (LPDIRECTDRAWENUMERATEEX) GetProcAddress(hdd,"DirectDrawEnumerateExA");
            if (lpDDEnumEx) {
              m_monitor_to_find=hm;
              lpDDEnumEx(&DDEnumCallbackEx, this, DDENUM_ATTACHEDSECONDARYDEVICES|DDENUM_NONDISPLAYDEVICES);
              if(m_found_devguid) {
                MONITORINFOEX mi;
                MEMSET(&mi,0,sizeof(mi));
                mi.cbSize=sizeof(mi);
                if (Gmi(hm,&mi)) {
                  m_mon_x=mi.rcMonitor.left;
                  m_mon_y=mi.rcMonitor.top;
                }
              }
            }
            FreeLibrary(hdd);
          }
        }
      }
      FreeLibrary(h);
    }
  }

  if(!m_found_devguid) DirectDrawCreate(NULL,&lpDD,NULL);
  else DirectDrawCreate(&m_devguid,&lpDD,NULL);

  if(!lpDD) {
    initing=false;
    return 0;
  }

  lpDD->SetCooperativeLevel(hwnd,DDSCL_NOWINDOWCHANGES|DDSCL_NORMAL);

  DDSURFACEDESC   ddsd;
  INIT_DIRECTDRAW_STRUCT(ddsd);
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
  HRESULT ddrval = lpDD->CreateSurface(&ddsd, &lpddsPrimary, NULL );

  // init overlay
  DDSURFACEDESC   ddsdOverlay;
  INIT_DIRECTDRAW_STRUCT(ddsdOverlay);
  ddsdOverlay.ddsCaps.dwCaps=DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
  ddsdOverlay.dwFlags= DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH;
  ddsdOverlay.dwWidth=w;
  ddsdOverlay.dwHeight=h;
  ddsdOverlay.lPitch=w*4;
  ddsdOverlay.dwBackBufferCount=0;
  DDPIXELFORMAT pf[]=
  {
    {sizeof(DDPIXELFORMAT),DDPF_FOURCC,MAKEFOURCC('Y','U','Y','2'),0,0,0,0,0},
    {sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('U','Y','V','Y'),0,0,0,0,0}, // UYVY
    {sizeof(DDPIXELFORMAT),DDPF_FOURCC,MAKEFOURCC('Y','V','1','2'),0,0,0,0,0},
  };
  int tab[5];
  if(!STRICMP(type,"YUY2")) {
    tab[0]=0; // default is YUY2
    tab[1]=1;
    tab[2]=-1;
  } else if(!STRICMP(type,"UYVY")) {
      tab[0]=1; // make UYVY default
      tab[1]=0;
      tab[2]=-1;
  } else if(!STRICMP(type,"YV12")) {
      tab[0]=2;
      tab[1]=0;
      tab[2]=1;
      tab[3]=-1;
  } else {
      tab[0]=-1; // default is RGB
  }

  int x=4096;
  HRESULT v=-1;
  for (x = 0; x < sizeof(tab)/sizeof(tab[0]) && tab[x]>=0; x ++) {
    ddsdOverlay.ddpfPixelFormat=pf[tab[x]];
    v=lpDD->CreateSurface(&ddsdOverlay, &lpddsOverlay, NULL);
    if (!FAILED(v)) break;
  }
  if(FAILED(v)||x>=sizeof(tab)/sizeof(tab[0])||tab[x]<0) {
    initing=false;
    return 0;
  }

  yuy2_output = (tab[x] == 0);
  uyvy_output = (tab[x] == 1);

  INIT_DIRECTDRAW_STRUCT(capsDrv);
  ddrval = lpDD->GetCaps(&capsDrv, NULL);

  uDestSizeAlign = capsDrv.dwAlignSizeDest;
  uSrcSizeAlign =  capsDrv.dwAlignSizeSrc;

  dwUpdateFlags = DDOVER_SHOW | DDOVER_KEYDESTOVERRIDE;

  DEVMODE d;
  d.dmSize=sizeof(d);
  d.dmDriverExtra=0;
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &d);

  int rv=OV_COL_R, gv=OV_COL_G, bv=OV_COL_B;

  INIT_DIRECTDRAW_STRUCT(ovfx);
  ovfx.dwDDFX=0;
  switch(d.dmBitsPerPel) {
    case 16:
      ovfx.dckDestColorkey.dwColorSpaceLowValue=((rv>>3) << 11) | ((gv>>2) << 5) | (bv>>3);
      break;
    case 15:
      ovfx.dckDestColorkey.dwColorSpaceLowValue=((rv>>3) << 10) | ((gv>>3) << 5) | (bv>>3);
      break;
    case 24: case 32:
      ovfx.dckDestColorkey.dwColorSpaceLowValue=(rv << 16) | (gv << 8) | bv;
      break;
  }
  ovfx.dckDestColorkey.dwColorSpaceHighValue=ovfx.dckDestColorkey.dwColorSpaceLowValue;

  getRects(&rs,&rd);
  if(FAILED(lpddsOverlay->UpdateOverlay(&rs, lpddsPrimary, &rd, dwUpdateFlags, &ovfx))) {
    initing=false;
    return 0;
  }
  initing=false;

  DDSURFACEDESC dd={sizeof(dd),};
  if (lpddsOverlay->Lock(NULL,&dd,DDLOCK_WAIT,NULL) != DD_OK) return 0;
  unsigned char *o=(unsigned char*)dd.lpSurface;
  if (uyvy_output||yuy2_output) 
  {
    int x=dd.lPitch*height/2;
    while (x--)
    {
      if (uyvy_output)
      {
        *o++=128;
        *o++=0;
      }
      else
      {
        *o++=0;
        *o++=-128;
      }
    }
  }
  else
  {
    MEMSET(o,0,dd.lPitch*height); o+=dd.lPitch*height;
    MEMSET(o,128,dd.lPitch*height/2);
  }
  lpddsOverlay->Unlock(&dd);

  parent->cascadeRepaint(); //draws the overlay color
  return 1;
}

void OverlayVideoOutput::getRects(RECT *drs, RECT *drd) {
  HWND hwnd=m_parent->gethWnd();

  RECT rd,rs;
  GetClientRect(hwnd,&rd);
  ClientToScreen(hwnd,(LPPOINT)&rd);
  ClientToScreen(hwnd,((LPPOINT)&rd) + 1);

  m_parent->adjustAspect(rd);
  rd.left-=m_mon_x;
  rd.right-=m_mon_x;
  rd.top-=m_mon_y;
  rd.bottom-=m_mon_y;

  MEMSET(&rs,0,sizeof(rs));
  rs.right=width;
  rs.bottom=height;

  //resize overlay for off-screen
  RECT rfull;
  Std::getViewport(&rfull,(HWND)NULL,1); //FUCKO: assume monitor 0
  if(rd.right>rfull.right) {
    int diff=rd.right-rfull.right;
    float sc=(float)(width)/(float)(rd.right-rd.left);
    rd.right=rfull.right;
    rs.right=width-(int)(diff*sc);
  }
  if(rd.left<rfull.left) {
    int diff=rfull.left-rd.left;
    float sc=(float)(width)/(float)(rd.right-rd.left);
    rd.left=rfull.left;
    rs.left=(int)(diff*sc);
  }
  if(rd.bottom>rfull.bottom) {
    int diff=rd.bottom-rfull.bottom;
    float sc=(float)(height)/(float)(rd.bottom-rd.top);
    rd.bottom=rfull.bottom;
    rs.bottom=height-(int)(diff*sc);
  }
  if(rd.top<rfull.top) {
    int diff=rfull.top-rd.top;
    float sc=(float)(height)/(float)(rd.bottom-rd.top);
    rd.top=rfull.top;
    rs.top=(int)(diff*sc);
  }

  if (capsDrv.dwCaps & DDCAPS_ALIGNSIZESRC && uDestSizeAlign) {
    rs.left = (int)((rs.left+uDestSizeAlign-1)/uDestSizeAlign)*uDestSizeAlign;
    rs.right = (int)((rs.right+uDestSizeAlign-1)/uDestSizeAlign)*uDestSizeAlign;
  }
  if (capsDrv.dwCaps & DDCAPS_ALIGNSIZEDEST && uDestSizeAlign) {
    rd.left = (int)((rd.left+uDestSizeAlign-1)/uDestSizeAlign)*uDestSizeAlign;
    rd.right = (int)((rd.right+uDestSizeAlign-1)/uDestSizeAlign)*uDestSizeAlign;
  }

  *drd=rd;
  *drs=rs;
}

void OverlayVideoOutput::timerCallback() {
  if(!m_parent) return;

  RECT rd,rs;
  getRects(&rs,&rd);

  if(MEMCMP(&m_oldrd,&rd,sizeof(RECT))) {
    m_oldrd=rd;
    if(!initing && lpddsOverlay) 
      if(FAILED(lpddsOverlay->UpdateOverlay(&rs, lpddsPrimary, &rd, dwUpdateFlags, &ovfx))) {
        needchange=1;
      }
  }
}

int OverlayVideoOutput::onPaint(Canvas *canvas, RECT &r) {
  if(!m_parent) return 0;

  canvas->fillRect(&r,RGB(OV_COL_R,OV_COL_G,OV_COL_B));

  return 1;
}

void OverlayVideoOutput::displayFrame(const char *buf, int size, int time) {
  if(!m_parent) return;
  
  DDSURFACEDESC dd={sizeof(dd),};
  if (vid_vsync) lpDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);
  HRESULT result;
  if ((result=lpddsOverlay->Lock(NULL,&dd,DDLOCK_WAIT,NULL)) != DD_OK) {
    //CT>FUCKO:reenable me (ctrl+alt+del on win2k)
    //if(result==DDERR_SURFACELOST) width=-1; //will try to recreate the surface in the next processData() call
    return;
  }
  if(!STRICMP(type,"YV12")) {
    const YV12_PLANES *planes=(YV12_PLANES *)buf;
    if (uyvy_output||yuy2_output) { // YV12planar->UYVY or YUY2
      unsigned char *o=(unsigned char*)dd.lpSurface;
      const unsigned char *yi=planes->y.baseAddr;
      const unsigned char *ui=planes->u.baseAddr;
      const unsigned char *vi=planes->v.baseAddr;
      int y=height;
      if (flip) o+=dd.lPitch*(height-1);
      while (y>0) {
        int x=width;
        unsigned char *oo=o;
      
        if (uyvy_output) while (x>0) {
          o[0]=*ui++; o[1]=*yi++; o[2]=*vi++; o[3]=*yi++;
          o+=4; x-=2;
        }
        else while (x>0) {
          o[0]=*yi++; o[1]=*ui++; o[2]=*yi++; o[3]=*vi++;
          o+=4; x-=2;
        }
        ui-=width/2;
        vi-=width/2;
        yi+=planes->y.rowBytes-width;
        x=width;
        if (flip) o=oo-dd.lPitch;
        else o+=dd.lPitch-width*2;
        oo=o;
        if (uyvy_output) while (x>0) {
          o[0]=*ui++; o[1]=*yi++; o[2]=*vi++; o[3]=*yi++;
          o+=4; x-=2;
        } else while (x>0) {
          o[0]=*yi++; o[1]=*ui++; o[2]=*yi++; o[3]=*vi++;
          o+=4; x-=2;
        }
        if (flip) o=oo-dd.lPitch;
        else o+=dd.lPitch-width*2;
        ui+=planes->u.rowBytes-(width/2);
        vi+=planes->v.rowBytes-(width/2);
        yi+=planes->y.rowBytes-width;
        y-=2;
      }
    } else { // woo native YV12 copy
      int f=!!flip;
      char *o=(char*)dd.lpSurface+(f*height*dd.lPitch);
      const char *i=(const char*)planes->y.baseAddr;
      int d_o=dd.lPitch;
      if (f) d_o=-d_o;
      else o-=d_o;
  
      int h2=height;
      while (h2--) {
        o+=d_o; MEMCPY(o,i,width); i+=planes->y.rowBytes;
      }

      d_o/=2;

      int w2=width/2;
      h2=height/2;
      i=(const char*)planes->v.baseAddr;
      o=(char*)dd.lpSurface+(height*dd.lPitch*(f+4))/4;

      if (!f) o-=d_o;
      while (h2--) {
        o+=d_o; MEMCPY(o,i,w2); i+=planes->v.rowBytes;
      }
      o=(char*)dd.lpSurface+(height*dd.lPitch*(f+5))/4;
      i=(const char*)planes->u.baseAddr;
      h2=height/2;

      if (!f) o-=d_o;
      while (h2--) {
        o+=d_o; MEMCPY(o,i,w2);i+=planes->u.rowBytes;
      }
    }
  } else if(!STRICMP(type,"YUY2") || !STRICMP(type,"UYVY")) {
    const char *a=buf;
    char *b=(char *)dd.lpSurface;
    int l=width*2,l2=dd.lPitch;
    if(flip) {
      b+=(height-1)*l2;
      l2=-l2;
    }
    int is_uyvy=STRICMP(type,"YUY2");
    if (uyvy_output && !is_uyvy || (yuy2_output && is_uyvy)) // convert to uyvy
    {
      for(int i=0;i<height;i++) {
        int x=width/2;
        while (x-->0) {
          b[0]=a[1];
          b[1]=a[0];
          b[2]=a[3];
          b[3]=a[2];
          a+=4;
          b+=4;          
        }
        MEMCPY(b,a,l);
        b+=l2;
        a+=l;
      }
    } else {
      //wee straight YUY2 copy
      for(int i=0;i<height;i++) {
        MEMCPY(b,a,l);
        b+=l2;
        a+=l;
      }
    }
  }

  lpddsOverlay->Unlock(&dd);
}

void OverlayVideoOutput::goFullScreen() {
  fullscreen_controls = new GuiObjectWnd;
  fullscreen_controls->setContent("video.fullscreen_controls");
  fullscreen_controls->init(m_parent);
  fullscreen_controls->setRectRgn(0);

  RECT r;
  Std::getViewport(&r,m_parent->gethWnd(),1);

  RECT nr = r;
  nr.top = 0;
  nr.bottom = (int)(r.bottom - r.top);
  fullscreen_controls->resizeToRect(&nr);

  is_fullscreen=1;
}

void OverlayVideoOutput::removeFullScreen() {
  delete fullscreen_controls;
  fullscreen_controls = NULL;
  is_fullscreen=0;
}

int OverlayVideoOutput::showOSD() {
  if (fullscreen_controls != NULL) fullscreen_controls->setVisible(TRUE);
  return 1;
}

void OverlayVideoOutput::hideOSD() { 
  if (fullscreen_controls != NULL) fullscreen_controls->setVisible(FALSE);
}
