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
#include "skinwnd.h"
#include "switchwnd.h"
#include "../common/notifmsg.h"
#include "../studio/wac.h"
#include "../common/msgbox.h"

SkinShotWnd::SkinShotWnd() {
  setName("ScreenShot");
  showLabel(TRUE);
  bitmap=NULL;
  tmpcanvas=NULL;
}

SkinShotWnd::~SkinShotWnd() {
  delete(bitmap);
  delete(tmpcanvas);
}

int SkinShotWnd::onInit() {
  SKINSHOT_PARENT::onInit();
  return 1;
}

void SkinShotWnd::setSkinItem(SkinItem *item) {
  delete(bitmap); bitmap=NULL;
  const char *shot=item->getScreenshot();
  if(shot) {
    StringPrintf tmp("%s%s%s/%s",api->getSkinsPath(),DIRCHARSTR,item->getName(),shot);
    bitmap = new AutoSkinBitmap(tmp);
  }
  onResize();
}

int SkinShotWnd::onPaint(Canvas *canvas)
{
  PaintCanvas paintcanvas;
  if (canvas == NULL) {
    if (!paintcanvas.beginPaint(this)) return 0;
    canvas = &paintcanvas;
  }
  SKINSHOT_PARENT::onPaint(canvas);
  RECT r;
  getClientRect(&r);
  if(tmpcanvas) tmpcanvas->blit(getScrollX(), getScrollY(), canvas, r.left, r.top, r.right-r.left, r.bottom-r.top);
	return 1;
}

int SkinShotWnd::getContentsWidth() {
  if(bitmap && bitmap->getBitmap()) return bitmap->getWidth();
  return 0;
}

int SkinShotWnd::getContentsHeight() {
  if(bitmap && bitmap->getBitmap()) return bitmap->getHeight();
  return 0;
}

int SkinShotWnd::onResize() {
  if(!isVisible()) return SKINSHOT_PARENT::onResize();

  delete(tmpcanvas);
  RECT r;
  getClientRect(&r);
  if(bitmap && bitmap->getBitmap()) {
    int w=max(bitmap->getWidth(),r.right);
    int h=max(bitmap->getHeight(),r.bottom);
    tmpcanvas=new BltCanvas(w,h);
    tmpcanvas->setBaseWnd(this);
    RECT r={0,0,w,h};
    renderBaseTexture(tmpcanvas, r);
    int bw=bitmap->getWidth();
    int bh=bitmap->getHeight();
    bitmap->getBitmap()->blit(tmpcanvas,(w-bw)/2,(h-bh)/2);
  } else {
    RECT nr={0,0,r.right,r.bottom};
    tmpcanvas=new BltCanvas(r.right,r.bottom);
    tmpcanvas->setBaseWnd(this);
    renderBaseTexture(tmpcanvas, nr);
  }
  invalidate();
  return SKINSHOT_PARENT::onResize();
}

int SkinShotWnd::onMouseWheelUp(int click, int lines) {
  if(needVScroll()) {
    int sy=getScrollY();
    sy-=10;
    if(sy<0) sy=0;
    scrollToY(sy);
  } else if(needHScroll()) {
    int sy=getScrollX();
    sy-=10;
    if(sy<0) sy=0;
    scrollToX(sy);
  }
  return 0;
}

int SkinShotWnd::onMouseWheelDown(int click, int lines) {
  if(needVScroll()) {
    int sy=getScrollY();
    sy+=10;
    if(sy>getMaxScrollY()) sy=getMaxScrollY();
    scrollToY(sy);
  } else if(needHScroll()) {
    int sy=getScrollX();
    sy+=10;
    if(sy>getMaxScrollX()) sy=getMaxScrollX();
    scrollToX(sy);
  }
  return 0;
}


SkinCommentWnd::SkinCommentWnd() {
  setDeleteChilds(0);
  
  setName("Skin infos");
  showLabel(TRUE);

  addChild(&nametext1,3,3,55,12);
  nametext1.setName("Name");
  nametext1.setTextSize(13);
  addChild(&nametext2,60,3,200,12);
  nametext2.setTextSize(13);
  nametext2.setCenter(0);

  addChild(&versiontext1,3,17,55,12);
  versiontext1.setName("Version");
  versiontext1.setTextSize(13);
  addChild(&versiontext2,60,17,200,12);
  versiontext2.setTextSize(13);
  versiontext2.setCenter(0);

  addChild(&commenttext1,3,31,55,12);
  commenttext1.setName("Comment");
  commenttext1.setTextSize(13);
  addChild(&commenttext2,60,31,200,12);
  commenttext2.setTextSize(13);
  commenttext2.setCenter(0);

  addChild(&authortext1,3,45,55,12);
  authortext1.setName("Author");
  authortext1.setTextSize(13);
  addChild(&authortext2,60,45,200,12);
  authortext2.setTextSize(13);
  authortext2.setCenter(0);

  addChild(&emailtext1,3,59,55,12);
  emailtext1.setName("Email");
  emailtext1.setTextSize(13);
  addChild(&emailtext2,60,59,200,12);
  emailtext2.setTextSize(13);
  emailtext2.setCenter(0);

  addChild(&homepagetext1,3,73,55,12);
  homepagetext1.setName("URL");
  homepagetext1.setTextSize(13);
  addChild(&homepagetext2,60,73,200,12);
  homepagetext2.setTextSize(13);
  homepagetext2.setCenter(0);

  addChild(&switchwnd, 3, 90, 55, 18);
  switchwnd.setName("Switch!");
  switchwnd.setNotifyId(0x1);

  addChild(&delwnd, 60, 90, 55, 18);
  delwnd.setName("Remove");
  delwnd.setNotifyId(0x2);
  
  addChild(&gotourlwnd, 117, 90, 55, 18);
  gotourlwnd.setName("Go to URL");
  gotourlwnd.setNotifyId(0x3);
  
  curitem = NULL;
}

void SkinCommentWnd::setSkinItem(SkinItem *item) {
  curitem=item;
  nametext2.setName(item->getFullName());
  versiontext2.setName(item->getVersion());
  commenttext2.setName(item->getComment());
  authortext2.setName(item->getAuthor());
  emailtext2.setName(item->getEmail());
  homepagetext2.setName(item->getHomepage());
}

int SkinCommentWnd::childNotify(RootWnd *child, int msg, int param1, int param2) {
	if(msg==ChildNotify::BUTTON_LEFTPUSH) {
    switch(child->getNotifyId()) {
    case 0x1:
      if(curitem) api->skin_switchSkin((char *)curitem->getName());
      break;
#if 0//CUT
    case 0x2: {
      // delete skin
      StringPrintf fn("%s%s%s.wal", api->getSkinPath, DIRCHARSTR, curitem->getName());
      if (api->messageBox("Are you sure you want to delete that skin?", fn, MSGBOX_YES|MSGBOX_NO, 0, this) == MSGBOX_YES) {
//CUT        api->messageBox("Well, too bad for you. This feature doesn't work yet :)","Oops",MSGBOX_OK,0,this);
        FDELETE(fn, FALSE);
      }
      break;
    }
#endif
    case 0x3: {
      const char *n=homepagetext2.getName();
      if(n && n[0]) api->main_navigateUrl(n);
    }
    break;
    }
    return 1;
  }
  return SKINCOMMENT_PARENT::childNotify(child,msg,param1,param2);
}

void SkinCommentWnd::resizeChild(BaseWnd *child) {
  RECT r;
  getClientRect(&r);
  RECT r2;
  child->getClientRect(&r2);
  r2.right=r.right-3;
  child->resize(&r2);
}
int SkinCommentWnd::onResize() {
  SKINCOMMENT_PARENT::onResize();

  resizeChild(&nametext2);
  resizeChild(&versiontext2);
  resizeChild(&commenttext2);
  resizeChild(&authortext2);
  resizeChild(&emailtext2);
  resizeChild(&homepagetext2);
  return 1;
}
