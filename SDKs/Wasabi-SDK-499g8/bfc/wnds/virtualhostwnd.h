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
#ifndef __VIRTUALHOSTWND_H
#define __VIRTUALHOSTWND_H

#include "../../common/guiobjwnd.h"

class VirtualHostWnd : public GuiObjectWnd {
  public:
    VirtualHostWnd();
    virtual ~VirtualHostWnd();

    virtual int onInit();
    virtual int onPaint(Canvas *c);
    virtual int onResize();
    virtual void onChildInvalidate(Region *r, RootWnd *who);

    virtual int onLeftButtonDown(int x, int y);
    virtual int onLeftButtonUp(int x, int y);
    virtual int onRightButtonDown(int x, int y);
    virtual int onRightButtonUp(int x, int y);
    virtual int onLeftButtonDblClk(int x, int y);
    virtual int onRightButtonDblClk(int x, int y);
    virtual int onMouseMove(int x, int y);

    virtual void virtualhostwnd_setContent(const char *groupid);
    virtual void virtualhostwnd_setContent(SkinItem *item);
    virtual void virtualhostwnd_onNewContent();
    virtual void virtualhostwnd_onPaintBackground(Canvas *c);

    virtual void virtualhostwnd_fitToClient(int fit);
    virtual void virtualhostwnd_getContentRect(RECT *r);
    virtual void virtualhostwnd_setScriptsEnabled(int en);

    virtual RootWnd *virtualhostwnd_getContentRootWnd();
#ifdef WASABI_COMPILE_SCRIPT
    virtual ScriptObject *virtualhostwnd_findScriptObject(const char *object_id);
#endif
#ifdef WASABI_COMPILE_SKIN
    virtual GuiObject *virtualhostwnd_findObject(const char *object_id);
    virtual GuiObject *virtualhostwnd_getContent();
    virtual ScriptObject *virtualhostwnd_getContentScriptObject();
#endif

  private:

    GuiObjectWnd *group;
    int fittoclient;
    int xoffset, yoffset;
    int groupwidth, groupheight;
    int scripts_enabled;
};

#endif
