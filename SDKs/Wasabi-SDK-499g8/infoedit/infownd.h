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
#ifndef _INFOWND_H
#define _INFOWND_H

#include <bfc/wnds/form/formwnd.h>
#include <core/corecb.h>
#include <studio/api.h>
#include <bfc/appcmds.h>
#include <common/script/c_script/h_guiobject.h>
#include <common/script/c_script/h_button.h>

class Info;
class ButtonWnd;
class ButtonClicksCallback;

class InfoWnd : public FormWnd, AppCmdsI {
public:
  InfoWnd();
  ~InfoWnd();

  virtual int onInit();

  virtual const char *getRootWndName() { return "Media Info editor"; }

  void updateInfo(const char *name, const char *value);
  void updateInfos();
  void commitChanges();
  
  void buttonClick(const char *id);

  virtual void appcmds_onCommand(int id, const RECT *buttonRect, int which_btn);

  // WCS:
  static const char *getWindowTypeName();
  static GUID getWindowTypeGuid();
  static void setIconBitmaps(ButtonWnd *button);

  //virtual int acceptExternalDrops() { return 1; }	// override this
  virtual int dragEnter(RootWnd *sourceWnd);
  virtual int dragDrop(RootWnd *sourceWnd, int x, int y);
  
  enum {
    CMD_UPDATE=0,
    CMD_UNDO,
    CMD_ADVANCED,
  };

private:
  Info *infos;
  PtrList<ButtonClicksCallback> buttonclickscallbacks;
};

class ButtonClicksCallback : public H_GuiObject {
  public:
    ButtonClicksCallback(ScriptObject *trap, InfoWnd *_callback, const char *_id) :
        callback(_callback), H_GuiObject(trap), id(_id) {
    }

    virtual void hook_onLeftButtonDown(int x, int y) {
      callback->buttonClick(id);
    }
  private:
    InfoWnd *callback;
    String id;
};

extern InfoWnd *infownd;

#endif//_INFOWND_H
