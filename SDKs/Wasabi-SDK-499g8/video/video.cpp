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
#include "video.h"
#include "videownd.h"
#include "videowac.h"

#include <bfc/parse/pathparse.h>
#include <bfc/attribs/attribs.h>

static int videonum;
static PtrList<Video> videoptrs;
static CriticalSection videocs;

Video::Video() {
  InCriticalSection cs(&videocs);
  videonum++;
  videoptrs.addItem(this);
  stop=0;
  wininit=0;
  ar_adjust = 1.0;
  apply_hardcoded_aspects = 0;
}

Video::~Video() {
  InCriticalSection cs(&videocs);
  videoptrs.removeItem(this);
  videonum--;
  if(!videonum && videownd)	{
    api->cmd_postCommand(videowac->getGUID(),"endvideo",0,0,NULL,0,0);
  }
}

void Video::stopPlayback() {
  InCriticalSection cs(&videocs);
  for(int i=0;i<videoptrs.getNumItems();i++) videoptrs[i]->setStop();
}

int Video::getInfos(MediaInfo *infos) {
  return 0;
}

int Video::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  InCriticalSection cs(&videocs);
  if(stop) return 0;
  Chunk *c=chunk_list->getChunk("VIDEO");
  if(c) {
    int l=c->getSize();
    if(!l) return 1;
    const char *type=(const char *)c->getInfo("type");
    if(type && isSupportedType(type)) {
      if(!wininit) {
        if(!videownd || (videownd&&!videownd->isVisible()) ) {
          // display the video window
          api->cmd_postCommand(videowac->getGUID(),"displaywindow",0,0,NULL,0,1);
        }
        if(!videownd) return 0;
        wininit=1;
      }// else if(videownd&&!videownd->isVisible()) return 0; // window has been closed
      int iar=c->getInfo("aspect");
      float ar = iar ? *((float*)&iar) : 1.f;

      int vw = c->getInfo("width"), vh = c->getInfo("height");

      if (apply_hardcoded_aspects || iar == 0) {
        PathParser pp(aspect_ratios, ";");
        for (int i = 0; i < pp.getNumStrings(); i++) {
          int aw=0, ah=0;
          float fr=0;
          SSCANF(pp.enumString(i), "%dx%d=%f", &aw, &ah, &fr);
          if (aw == vw && ah == vh && fr > 0.f) {
            ar = fr;
            break;
          }
        }
      }

      ar *= ar_adjust;

      if(!videownd->setSize(vw, vh,type,c->getInfo("flipv"),ar))
        return 0;
      videownd->setPalette((RGBQUAD *)c->getInfo("palette")); //for RGB8
      videownd->addFrame((const char *)c->getData(),l,c->getInfo("time"));
    }
  }
  return 1;
}

int Video::corecb_onStarted() {
  // adjust aspect ratio to physical monitor
  ar_adjust = 1.0;

  aspect_ratios = NULL;
  apply_hardcoded_aspects = 0;

  //this component
  // {F0816D7B-FFFC-4343-80F2-E8199AA15CC3}
  const GUID guid = 
  { 0xf0816d7b, 0xfffc, 0x4343, { 0x80, 0xf2, 0xe8, 0x19, 0x9a, 0xa1, 0x5c, 0xc3 } };
  CfgItem *ci = WASABI_API_CONFIG->config_getCfgItemByGuid(guid);
  char buf[WA_MAX_PATH];
  if (ci->getData("Default aspect ratios", buf, sizeof buf)) {
    aspect_ratios = buf;
  }
  apply_hardcoded_aspects = ci->getDataAsInt("Force default aspect ratios");

  const GUID options_guid =
    { 0x280876cf, 0x48c0, 0x40bc, { 0x8e, 0x86, 0x73, 0xce, 0x6b, 0xb4, 0x62, 0xe5 } };
  ci = WASABI_API_CONFIG->config_getCfgItemByGuid(options_guid);
  if (ci != NULL) {
    char bleh[512]="4:3";
    int r = ci->getData("Monitor aspect ratio", bleh, 512);
    if (r > 0) {
      PathParser pp(bleh, ":");
      if (pp.getNumStrings() == 2) {
        float marn = (float)ATOF(pp.enumString(0));
        float mard = (float)ATOF(pp.enumString(1));
        if (marn >= 1.f && mard >= 1.f) {
          float sw = (float)Std::getScreenWidth(), sh = (float)Std::getScreenHeight();
          ar_adjust = (marn / sw) / (mard / sh);
        }
      }
    }
  }
  return 1;
}

int Video::isSupportedType(const char *type) {
  extern _bool vid_support_yuy2;
  if(!STRICMP(type,"YUY2") && vid_support_yuy2) return 1;
  extern _bool vid_support_uyvy;
  if(!STRICMP(type,"UYVY") && vid_support_uyvy) return 1;
  extern _bool vid_support_yv12;
  if(!STRICMP(type,"YV12") && vid_support_yv12) return 1;

  if(!STRICMP(type,"RGB32")) return 1;
  if(!STRICMP(type,"RGB24")) return 1;
  if(!STRICMP(type,"RGB8")) return 1;

  if(!STRICMP(type,"SELF_DRAW")) return 1;

  return 0;
}
