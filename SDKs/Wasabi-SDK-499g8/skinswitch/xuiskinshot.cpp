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
#include "xuiskinshot.h"
#include "xuiskinlist.h"

// -----------------------------------------------------------------------
char SkinShotXuiObjectStr[] = "SkinSwitch:Preview"; // This is the xml tag
char SkinShotXuiSvcName[] = "SkinSwitch:Preview XuiObject Service";

// -----------------------------------------------------------------------
XuiSkinShot::XuiSkinShot() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "", SKINSHOT_SET, XUI_ATTRIBUTE_IMPLIED);
  getScriptObject()->vcpu_setInterface(skinshotGuid, this);
  
  bitmap=NULL;
}

XuiSkinShot::~XuiSkinShot() {
  delete(bitmap);
}

// -----------------------------------------------------------------------
int XuiSkinShot::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SKINSHOT_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case SKINSHOT_SET:
      set(value);
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void XuiSkinShot::set(const char *elementname) {
}

// -----------------------------------------------------------------------
void XuiSkinShot::setSkinItem(SkinItem *item) {
  delete(bitmap); bitmap=NULL;
  const char *shot=item->getScreenshot();
  if(shot) {
    StringPrintf tmp("%s%s%s/%s",api->getSkinsPath(),DIRCHARSTR,item->getName(),shot);
    bitmap = new SkinBitmap(tmp);
  }
  invalidate();
}

// -----------------------------------------------------------------------
int XuiSkinShot::onPaint(Canvas *canvas)
{
  PaintCanvas paintcanvas;
  if (canvas == NULL) {
    if (!paintcanvas.beginPaint(this)) return 0;
    canvas = &paintcanvas;
  }
  SKINSHOT_PARENT::onPaint(canvas);
  RECT r;
  getClientRect(&r);

  if (r.right <= r.left || r.bottom <= r.top) return 1; 

  if (bitmap && bitmap->getWidth() > 0 && bitmap->getHeight() > 0) {
    RECT dst;
    int w = r.right-r.left;
    int h = r.bottom-r.top;
    if (w > bitmap->getWidth() && h > bitmap->getHeight()) { // no resize, there's plenty of room, just center it
      dst.left = r.left + ((w - bitmap->getWidth())/2);
      dst.top = r.top + ((h - bitmap->getHeight())/2);
      dst.right = dst.left + bitmap->getWidth();
      dst.bottom = dst.top + bitmap->getHeight();
    } else {
      float vh = (float)h / bitmap->getHeight();
      float vw = (float)w / bitmap->getWidth();
      if (vw > vh) { // wider than tall, resize according to height
        dst.left = r.left + ((w - (int)((float)bitmap->getWidth()*vh))/2);
        dst.top = r.top;
        dst.right = dst.left + (int)((float)bitmap->getWidth()*vh);
        dst.bottom = dst.top + h;
      } else { // taller than wide, resize according to width
        dst.left = r.left;
        dst.top = r.top + ((h - (int)((float)bitmap->getHeight()*vw))/2);
        dst.right = dst.left + w;
        dst.bottom = dst.top + (int)((float)bitmap->getHeight()*vw);
      }
    }
    bitmap->stretchToRectAlpha(canvas, &dst, getPaintingAlpha());
  }
	return 1;
}

