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
#include "spywnd.h"
#include "../../bfc/canvas.h"

// -----------------------------------------------------------------------

// These are the strings used by the xuiobject service (see spywnd.h)

char SpyWndXuiObjectStr[] = "¨PaintSpy:Spy"; // This is the xml tag
char SpyWndXuiSvcName[] = "¨PaintSpy:Spy XuiObject Service"; // and this is the name of the service

// -----------------------------------------------------------------------

// This is the actual object that you are instantiating each time your xml tag is 
// hit by the xml parser, it simply inherits from XuiObject (from GuiObjectWnd, from ClickWnd). 
// Don't forget to call your ancestor when overriding events!

SpyWnd::SpyWnd() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "image", SpyWnd_SETIMAGE, XUI_ATTRIBUTE_IMPLIED);
//  addParam("someotheroption",  SpyWnd_SOMEOTHEROPTION, XUI_ATTRIBUTE_REQUIRED);
//  addParam("someotheroption",  SpyWnd_SOMEOTHEROPTION, XUI_ATTRIBUTE_REQUIRED);
}

SpyWnd::~SpyWnd() {
}

// XuiObject automatically calls this back for all parameters registered using addParam
// encountered in the xml source
int SpyWnd::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return SpyWnd_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case  SpyWnd_SETIMAGE:
      setImage(value);
      break;
/*    case  SpyWnd_SOMEOTHEROPTION:
      setSomeOtherOption(value);
      break;
    case  SpyWnd_SOMEOTHEROPTION:
      setSomeOtherOption(value);
      break;*/
    default:
      return 0;
  }
  return 1;
}

// Let's just act as a layer
int SpyWnd::onPaint(Canvas *canvas) {
  SpyWnd_PARENT::onPaint(canvas);
  if (image.getBitmap()) {
    RECT r;
    getClientRect(&r);
    image.getBitmap()->stretchToRectAlpha(canvas, &r, getPaintingAlpha()); // try to always use getPaintingAlpha when bliting in your client area
  }
  return 1;
}

// Here we set some private data
void SpyWnd::setImage(const char *elementname) {
  image = elementname; // this sets this element name in autoskinbitmap
  if (isInited()) invalidate(); // update our rectangle if we're alive
}

// Here we set some more private data
//void SpyWnd::setSomeMoreOption(const char *paramvalue) {
//}

// This allows automatic w/h parameters in your xuiobject
int SpyWnd::getPreferences(int what) {
  switch (what) {
    case SUGGESTED_W:
      if (image.getBitmap()) return image.getBitmap()->getWidth();
    case SUGGESTED_H:
      if (image.getBitmap()) return image.getBitmap()->getHeight();
  }
  return SpyWnd_PARENT::getPreferences(what);
}

