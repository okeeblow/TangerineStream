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
#include "cfg_actions.h"
#include "../bfc/std.h"
#include "../bfc/wnds/skinwnd.h"
#include "../common/nsguid.h"
#include <bfc/attribs/cfgitem.h>
#include "../studio/api.h"
#include "preferences.h"

ConfigActionsSvc::ConfigActionsSvc() {
  registerAction("Preferences:OPEN", ACTION_PREFERENCESOPEN);
  registerAction("Preferences:OPENMODAL", ACTION_PREFERENCESOPENMODAL);
}

int ConfigActionsSvc::onActionId(int id, const char *action, const char *param, int p1, int p2, void *data, int datalen, RootWnd *source) {
  return performAction(id, param, p1, p2, data, datalen, source);
}

int ConfigActionsSvc::performAction(int actionid, const char *param, int p1, int p2, void *data, int datalen, RootWnd *source) {
  switch (actionid) {
    case ACTION_PREFERENCESOPENMODAL: {
      GUID g = nsGUID::fromChar(param);
      GUID gg;
      const char *grp;
      for (int i=0;i<api->preferences_getNumGroups();i++) {
        grp = api->preferences_enumGroup(i, NULL, &gg, NULL);
        if (gg == g) {
          SkinWnd s(grp);
          s.destroy();
          return 1;
        }
      }
      CfgItem *item = WASABI_API_CONFIG->config_getCfgItemByGuid(g);
      const char *xml = item->getConfigXML();
      if (xml && *xml) {
        SkinWnd s(xml);
        s.destroy();
      }
      return 1;
    }
    case ACTION_PREFERENCESOPEN: {
      GUID g = nsGUID::fromChar(param);
      GUID gg;
      const char *grp;
      for (int i=0;i<api->preferences_getNumGroups();i++) {
        grp = api->preferences_enumGroup(i, NULL, &gg, NULL);
        if (gg == g) {
          preferenceswac->openGroup(grp);
          return 1;
        }
      }
      CfgItem *item = WASABI_API_CONFIG->config_getCfgItemByGuid(g);
      const char *xml = item->getConfigXML();
      if (xml && *xml) {
        preferenceswac->openGroup(grp);
      }
      return 1;
    }
  }
  return 0;
}