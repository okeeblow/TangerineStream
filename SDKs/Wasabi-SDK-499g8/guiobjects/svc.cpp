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
#include "svc.h"
#include "xuibrowser.h"
#include "xuiquerylist.h"
#include "xuifilterlist.h"
#include "xuiwndholder.h"
#include "../bfc/wnds/dropdownlist.h"
#include "../bfc/wnds/embeddedxui.h"
#include "xuistatus.h"
#include "xuitabsheet.h"
#include "xuilist.h"
#include "xuitree.h"
#include "xuicheckbox.h"
#include "xuiformwnd.h"

// -----------------------------------------------------------------------------------------------------
// Service

ScriptObjectController *controllers[] = { 
  embeddedXuiController, // keep around the top please
  browserController, 
  queryListController,
  filterListController,
  windowHolderController,
  dropDownListController,
  layoutStatusController,
  tabsheetController,
  guiListController,
  guiTreeController,
  treeItemController,
  checkBoxController,
  xuiFormWndScriptController,
};

ScriptObjectController *ScriptObjectSvc::getController(int n) {
  if (n >= sizeof(controllers)/sizeof(ScriptObjectController *)) return NULL;
  return controllers[n];
}

