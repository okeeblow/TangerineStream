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
#include "crop.h"

#include <bfc/std.h>
#include <bfc/attribs/attribs.h>

int Crop::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  extern _bool vid_crop169;
  if (!vid_crop169) return 1;
  Chunk *c=chunk_list->getChunk("VIDEO");
  if(c) {
    int l=c->getSize();
    if(!l) return 1;
    const char *type=(const char *)c->getInfo("type");
    if (!checkType(type)) return 1;

    int w = c->getInfo("width"), h = c->getInfo("height");
#define COVERAGE .70	// .75 is more correct but .70 looks better
    int croph = (int)((float)h * (1.-COVERAGE))/2;
    void *ptr = static_cast<ARGB32 *>(c->getData());
    int cl = croph * w;
    extern _bool vid_cropdim;
    if (!vid_cropdim) {	// just put bars
      if (STREQL(type, "RGB32") || STREQL(type, "RGB24")) {
        ARGB32 *argb32 = static_cast<ARGB32*>(ptr);
        MEMFILL<ARGB32>(argb32, 0, cl);
        MEMFILL<ARGB32>(argb32 + ((w * h) - cl), 0, cl);
      } else if (STREQL(type, "RGB565") || STREQL(type, "YUY2")) {
        // hee, hee need to figure out what is black for YUY2 --BU
        RGB16 *rgb16 = static_cast<RGB16*>(ptr);
        MEMFILL<RGB16>(rgb16, 0, cl);
        MEMFILL<RGB16>(rgb16 + ((w * h) - cl), 0, cl);
      }	// need to handle more types
    } else {
      c->getChunkInfos()->addInfo("height", h-croph*2);
//      if (c->getInfo("height") != h-croph*2)
  //      MessageBox(NULL,"Fuckup","A",0);
      if (STREQL(type, "RGB32") || STREQL(type, "RGB24")) {
        ARGB32 *argb32 = static_cast<ARGB32*>(ptr);
        MEMCPY(argb32, argb32 + cl, (w * h - cl - cl) * sizeof(ARGB32));
      } else if (STREQL(type, "RGB565") || STREQL(type, "YUY2")) {
        RGB16 *rgb16 = static_cast<RGB16*>(ptr);
        MEMCPY(rgb16, rgb16 + cl, (w * h - cl - cl) * sizeof(ARGB16));
      }	else if (STREQL(type,"YV12")) {
        YV12_PLANES *p=static_cast<YV12_PLANES*>(ptr);
        p->y.baseAddr+=croph*p->y.rowBytes;
        p->u.baseAddr+=(croph/2)*p->u.rowBytes;
        p->v.baseAddr+=(croph/2)*p->v.rowBytes;
      } // need to handle more types
    }
  }
  return 1;
}

int Crop::checkType(const char *type) {
  if (type == NULL) return 0;
  if (STREQL(type, "RGB32") ||
      STREQL(type, "RGB24") ||
      STREQL(type, "RGB565") ||
      STREQL(type, "YV12") ||
      STREQL(type, "YUY2") ) return 1;
  return 0;
}

