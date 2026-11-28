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
#ifndef _SHADOW_H
#define _SHADOW_H

#include "../common/guiobjwnd.h"
#include "../bfc/paintcb.h"

#define XUISHADOWWND_PARENT GuiObjectWnd
class XuiShadowWnd : public XUISHADOWWND_PARENT, public PaintCallback {
public:
  static const char *xuiobject_getXmlTag() { return "Shadow"; }

  XuiShadowWnd();
  virtual ~XuiShadowWnd();

  virtual int onInit();

  virtual int onPaint(Canvas *canvas);

  virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

  virtual void onAfterPaint(PaintCallbackInfo *info);
  virtual void onInvalidation(PaintCallbackInfo *info);
protected:
  virtual void onWindowDeleted(RootWnd *w);
  void attachToGroup();
  virtual void timerclient_timerCallback(int id);

private:
  int myxuihandle;
  String targetname;
  RootWnd *group;
  BltCanvas *bltcanvas;
  int c_w, c_h;
  int in_paint;
};

class XuiShadowWndSvc : public XuiObjectSvc2<XuiShadowWnd> {};

#endif
