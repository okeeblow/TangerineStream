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
#ifndef _SKINWND_H
#define _SKINWND_H

#include "../common/contwnd.h"
#include "../common/textbar.h"
#include "../common/buttwnd.h"
#include "../common/scbkgwnd.h"

class SkinItem;

#define SKINSHOT_PARENT ScrlBkgWnd
class SkinShotWnd : public SKINSHOT_PARENT/*, public DependentViewerI*/ {
public:
  SkinShotWnd();
  ~SkinShotWnd();

  virtual int onInit();
  virtual int onPaint(Canvas *canvas);

  void setSkinItem(SkinItem *item);

  virtual int getContentsWidth();
  virtual int getContentsHeight();
  virtual int onResize();

  virtual int onMouseWheelUp(int click, int lines);
  virtual int onMouseWheelDown(int click, int lines);
  
private:
	AutoSkinBitmap *bitmap;
  BltCanvas *tmpcanvas;
};

#define SKINCOMMENT_PARENT ContWnd
class SkinCommentWnd : public SKINCOMMENT_PARENT {
public:
  SkinCommentWnd();
  void setSkinItem(SkinItem *item);

  virtual int childNotify(RootWnd *child, int msg, int param1=0, int param2=0);

  void resizeChild(BaseWnd *c);
  virtual int onResize();

private:
  TextBar nametext1,nametext2;
  TextBar versiontext1,versiontext2;
  TextBar commenttext1,commenttext2;
  TextBar authortext1,authortext2;
  TextBar emailtext1,emailtext2;
  TextBar homepagetext1,homepagetext2;

  ButtonWnd switchwnd,delwnd,gotourlwnd;

  SkinItem *curitem;
};

#endif
