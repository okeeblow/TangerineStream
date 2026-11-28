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
#include "xuiquerydrag.h"
#include "../bfc/canvas.h"
#include "../bfc/db/multiqueryserver.h"
#include "../common/filename.h"

char QueryDragXuiObjectStr[] = "QueryDrag"; // This is the xml tag
char QueryDragXuiSvcName[] = "QueryDrag XuiObject Service"; // and this is the name of the service

QueryDrag::QueryDrag() {
  setVirtual(0); // fucko
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "image", QUERYDRAG_SETIMAGE, XUI_ATTRIBUTE_IMPLIED);
  addParam(myxuihandle, "source", QUERYDRAG_SETSOURCE, XUI_ATTRIBUTE_IMPLIED);
  fn = NULL;
}

QueryDrag::~QueryDrag() {
}

int QueryDrag::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return QUERYDRAG_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case  QUERYDRAG_SETIMAGE:
      setImage(value);
      break;
    case  QUERYDRAG_SETSOURCE:
      setSource(value);
      break;
    default:
      return 0;
  }
  return 1;
}

int QueryDrag::onPaint(Canvas *canvas) {
  QUERYDRAG_PARENT::onPaint(canvas);

  RECT r;
  getClientRect(&r);

  RenderBaseTexture(canvas, r, 255);

  if (image.getBitmap()) 
    image.getBitmap()->stretchToRectAlpha(canvas, &r, getPaintingAlpha()); 

  return 1;
}

void QueryDrag::setImage(const char *elementname) {
  image = elementname; 
  if (isInited()) invalidate(); 
}

void QueryDrag::setSource(const char *elementname) {
  source = elementname;
}

int QueryDrag::getPreferences(int what) {
  switch (what) {
    case SUGGESTED_W:
      if (image.getBitmap()) return image.getBitmap()->getWidth();
    case SUGGESTED_H:
      if (image.getBitmap()) return image.getBitmap()->getHeight();
  }
  return QUERYDRAG_PARENT::getPreferences(what);
}

int QueryDrag::onMouseMove(int x, int y) {
  QUERYDRAG_PARENT::onMouseMove(x,y);
  if (isInClick()) 
    onBeginDrag();
  return 1;
}

void QueryDrag::onBeginDrag() {
  RootWnd *mqsw = NULL;
  if (source.isempty()) mqsw = findWindowByInterface(multiQueryServerGuid);
  else mqsw = findWindow(source);
  if (!mqsw) return;
  MultiQueryServer *mqs = static_cast<MultiQueryServer *>(mqsw->getInterface(multiQueryServerGuid));
  
  // multiquery is now available in mqs->mqs_getMultiQuery(); using format "table guid;query;table guid;query;etc..."
  fn = new FilenameI(StringPrintf("query://%s.nsq", mqs->mqs_getMultiQuery()));
  addDragItem(Filename::dragitem_getDatatype(), static_cast<Filename*>(fn));
  handleDrag();
}

int QueryDrag::dragComplete(int success) {
  ASSERT(fn != NULL);
  delete fn; fn = NULL;
  return 1;
}
