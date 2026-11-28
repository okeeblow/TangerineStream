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

#include "actioner.h"

#include <bfc/parse/cmdline.h>
#include <bfc/paramparser.h>
#include <common/nsguid.h>
#include <bfc/attribs/attribute.h>
#include <studio/services/svc_action.h>

#define ACTIONPREFIX "action"
#define CFGPREFIX "attrset"

void Actioner::onRegisterServices() {
  api->syscb_registerCallback(this);
}

void Actioner::onDeregisterServices() {
  api->syscb_deregisterCallback(this);
}

int Actioner::cmdlinecb_onProcess(const char *cmdline, int seqno) {
  CmdLine cmd(cmdline);
  if (cmd.hasSwitch(ACTIONPREFIX)) {
    FireAction x(cmd.getSwitchParam(ACTIONPREFIX, 0), cmd.getSwitchParam(ACTIONPREFIX, 1));
  }
  // /attrset={guid};attrname,value
  if (cmd.hasSwitch(CFGPREFIX)) {
    // {guid};attrname
    ParamParser pp(cmd.getSwitchParam(CFGPREFIX, 0));
    if (pp.getNumItems() == 2) {
      GUID g;
      g = nsGUID::fromChar(pp.enumItem(0));
      if (g != INVALID_GUID) {
        CfgItem *cfg = WASABI_API_CONFIG->config_getCfgItemByGuid(g);
        if (cfg != NULL) {
          int type = cfg->getAttributeType(pp.enumItem(1));
          String val = cmd.getSwitchParam(CFGPREFIX, 1);
          if ((type == AttributeType::BOOL || type == AttributeType::INT) && !STRNNCMP(val, "!")) {//toggle bool/int
            PathParser zopp(val, "!");
            const char *one = zopp.enumString(1);
            if (one == NULL) one = "1";
            const char *zero = zopp.enumString(2);
            if (zero == NULL) zero = "0";
            int prev = cfg->getDataAsInt(pp.enumItem(1));
            if (prev == 0) val = one;
            else val = zero;
          }
          cfg->setData(pp.enumItem(1), val);
        }
      }
    }
  }
  return 0;
}
