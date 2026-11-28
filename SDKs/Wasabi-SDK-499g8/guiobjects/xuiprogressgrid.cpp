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
#include "xuiprogressgrid.h"
#include "../bfc/canvas.h"
#include "../bfc/stringtable.h"
#include "../bfc/api/api_core.h"

#define ProgressGrid_TIMER_POS 1
#define PROGRESSGRID_INTERVAL 500

#ifndef _WASABIRUNTIME

BEGIN_SERVICES(ProgressGrid_Svc);
DECLARE_SERVICE(XuiObjectCreator<ProgressGridXuiSvc>);
END_SERVICES(ProgressGrid_Svc, _ProgressGrid_Svc);

#ifdef _X86_
extern "C" { int _link_ProgressGridXuiSvc; }
#else
extern "C" { int __link_ProgressGridXuiSvc; }
#endif

#endif


BEGIN_STRINGTABLE(_pgorientationvalues)
STI("top", PROGRESSGRID_TOP);
STI("left", PROGRESSGRID_LEFT);
STI("right", PROGRESSGRID_RIGHT);
STI("bottom", PROGRESSGRID_BOTTOM);
END_STRINGTABLE(_pgorientationvalues, pgorientationvalues)

// -----------------------------------------------------------------------

char ProgressGridXuiObjectStr[] = "ProgressGrid"; // xml tag
char ProgressGridXuiSvcName[] = "ProgressGrid XuiObject Service"; 

// -----------------------------------------------------------------------
ProgressGrid::ProgressGrid() {
  myxuihandle = newXuiHandle();
  addParam(myxuihandle, "orientation",  PROGRESSGRID_SETORIENTATION,  XUI_ATTRIBUTE_IMPLIED);
  orientation = PROGRESSGRID_RIGHT;
  progress = 0;
  started = 0;
}

// -----------------------------------------------------------------------
ProgressGrid::~ProgressGrid() {
  killTimer(ProgressGrid_TIMER_POS);
  WASABI_API_MEDIACORE->core_delCallback(0, this);
}

// -----------------------------------------------------------------------
int ProgressGrid::setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value) {
  if (xuihandle != myxuihandle)
    return PROGRESSGRID_PARENT::setXuiParam(xuihandle, xmlattributeid, xmlattributename, value);

  switch (xmlattributeid) {
    case PROGRESSGRID_SETORIENTATION:
      setOrientation(value);
      break;
    default:
      return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------
void ProgressGrid::setOrientation(const char *or) {
  int a = pgorientationvalues.getId(or);
  if (orientation == a) return;
  orientation = a;
  invalidate();
}

// -----------------------------------------------------------------------
void ProgressGrid::getGridRect(RECT *r) {
  RECT cr;
  getClientRect(&cr);
  float p = started ? progress : 0.0f;
  int height = (int)((float)(cr.bottom - cr.top) * p);
  int width = (int)((float)(cr.right - cr.left) * p);

  switch (orientation) {
    case PROGRESSGRID_LEFT: cr.left = cr.right - width; break;
    case PROGRESSGRID_TOP:  cr.top = cr.bottom - height; break;
    case PROGRESSGRID_RIGHT: cr.right = cr.left + width; break;
    case PROGRESSGRID_BOTTOM: cr.bottom = cr.top + height; break;
  }
  *r = cr;
}

// -----------------------------------------------------------------------
int ProgressGrid::onInit() {
  PROGRESSGRID_PARENT::onInit();
  timerCallback(ProgressGrid_TIMER_POS);
  setTimer(ProgressGrid_TIMER_POS, PROGRESSGRID_INTERVAL);
  WASABI_API_MEDIACORE->core_addCallback(0, this);
  if (WASABI_API_MEDIACORE->core_getStatus(0) != 0) started = 1; else started = 0;
  return 1;
}

// -----------------------------------------------------------------------
int ProgressGrid::corecb_onSeeked(int newpos) {
  int len = WASABI_API_MEDIACORE->core_getLength(0);
  if (newpos == -1) setProgress(0);
  else setProgress(((float)newpos/ (float)len));
  return 0;
}

// -----------------------------------------------------------------------
int ProgressGrid::corecb_onStarted() {
  started = 1;
  invalidate();
  return 0;
}

// -----------------------------------------------------------------------
int ProgressGrid::corecb_onStopped() {
  started = 0;
  progress = 0.0f;
  invalidate();
  return 0;
}

void ProgressGrid::setProgress(float p) {
  if (progress == p) return;
  progress = p;
  invalidate();
}

void ProgressGrid::timerCallback(int id) {
  switch (id) {
    case ProgressGrid_TIMER_POS: {
	    int playpos = WASABI_API_MEDIACORE->core_getPosition(0);
      corecb_onSeeked(playpos);
    }
    break;
  default:
    PROGRESSGRID_PARENT::timerCallback(id);
  }
}
