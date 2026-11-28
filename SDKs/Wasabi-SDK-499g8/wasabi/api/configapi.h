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
#ifndef __CONFIG_API_H
#define __CONFIG_API_H

#include "../../bfc/api/api_configi.h"

class ConfigApi : public config_apiI {
  public:
    ConfigApi() {}
    virtual ~ConfigApi() {}

    virtual void config_registerCfgItem(CfgItem *cfgitem);
    virtual void config_deregisterCfgItem(CfgItem *cfgitem);
    virtual int config_getNumCfgItems();
    virtual CfgItem *config_enumCfgItem(int n);
    virtual CfgItem *config_getCfgItemByGuid(GUID g);
    virtual void setIntPrivate(const char *name, int val);
    virtual int getIntPrivate(const char *name, int def_val);
    virtual void setIntArrayPrivate(const char *name, const int *val, int nval);
    virtual int getIntArrayPrivate(const char *name, int *val, int nval);
    virtual void setStringPrivate(const char *name, const char *str);
    virtual int getStringPrivate(const char *name, char *buf, int buf_len, const char *default_str);
    virtual int getStringPrivateLen(const char *name);
    virtual void setIntPublic(const char *name, int val);
    virtual int getIntPublic(const char *name, int def_val);
    virtual void setStringPublic(const char *name, const char *str);
    virtual int getStringPublic(const char *name, char *buf, int buf_len, const char *default_str);

  protected:
};


#endif
