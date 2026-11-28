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
#ifndef __API_CONFIG_IMPL_H
#define __API_CONFIG_IMPL_H

/*<?<autoheader/>*/
#include "api_config.h"
#include "api_configx.h"

class CfgItem;
class ConfigFile;
/*?>*/

class api_configI : public api_configX {
public:
  api_configI();
  virtual ~api_configI();

  DISPATCH(10) virtual void config_registerCfgItem(CfgItem *cfgitem);
  DISPATCH(20) virtual void config_deregisterCfgItem(CfgItem *cfgitem);
  DISPATCH(30) virtual int config_getNumCfgItems();
  DISPATCH(40) virtual CfgItem *config_enumCfgItem(int n);
  DISPATCH(50) virtual CfgItem *config_getCfgItemByGuid(GUID g);
  DISPATCH(60) virtual void setIntPrivate(const char *name, int val);
  DISPATCH(70) virtual int getIntPrivate(const char *name, int def_val);
  DISPATCH(80) virtual void setIntArrayPrivate(const char *name, const int *val, int nval);
  DISPATCH(90) virtual int getIntArrayPrivate(const char *name, int *val, int nval);
  DISPATCH(100) virtual void setStringPrivate(const char *name, const char *str);
  DISPATCH(110) virtual int getStringPrivate(const char *name, char *buf, int buf_len, const char *default_str);
  DISPATCH(120) virtual int getStringPrivateLen(const char *name);
  DISPATCH(130) virtual void setIntPublic(const char *name, int val);
  DISPATCH(140) virtual int getIntPublic(const char *name, int def_val);
  DISPATCH(150) virtual void setStringPublic(const char *name, const char *str);
  DISPATCH(160) virtual int getStringPublic(const char *name, char *buf, int buf_len, const char *default_str);
private:
  ConfigFile * public_config;
};

/*[interface.footer.h]
// {470B890C-4747-4113-ABA5-A8F8F4F8AB7E}
static const GUID configApiServiceGuid = 
{ 0x470b890c, 0x4747, 0x4113, { 0xab, 0xa5, 0xa8, 0xf8, 0xf4, 0xf8, 0xab, 0x7e } };

extern api_config *configApi;
*/

#endif // __API_CONFIG_IMPL_H
