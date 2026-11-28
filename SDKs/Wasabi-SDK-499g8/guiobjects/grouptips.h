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
#ifndef __GROUPTIPS_H
#define __GROUPTIPS_H

#include "../studio/services/svc_tooltips.h"
#include "../bfc/ptrlist.h"
#include "../bfc/timerclient.h"

class RootWnd;

int groupTipsTimerProc(int id, void *data1, void *data2);

class GroupTips : public svc_toolTipsRendererI/*, public TimerClientI*/ {
public:
  GroupTips();
  virtual ~GroupTips();
  static const char *getServiceName() { return "Group Tooltips Renderer"; }
  
  virtual int spawnTooltip(const char *text);
/*  virtual void timerCallback(int id);  

  static void shutdown();
  static void onTimer();


private:

  static int _spawnTooltip(const char *text);
  static void _killTooltip(int id);

  static PtrList<ToolTip> tooltips;
  static int count; // only grows*/
private:
  RootWnd *tipwnd;
};

#endif
