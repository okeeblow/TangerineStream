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
#ifndef _SHOUTCASTWAC_H
#define _SHOUTCASTWAC_H

#include "../studio/wac.h"
#include "../common/rootcomp.h"
#include "../attribs/cfgitemi.h"
#include "../attribs/attrint.h"
#include "../attribs/attrbool.h"

#define WACNAME WACshoutcast

#define SC_VERSION "0.9"

class WACNAME : public WAComponentClient {
public:
  WACNAME();
  virtual ~WACNAME();

  virtual void onDestroy();
  
  virtual const char *getName() { return "Shoutcaster v" SC_VERSION; };
  virtual GUID getGUID();

  virtual void onRegisterServices();

  virtual int getDisplayComponent() { return FALSE; };

  virtual CfgItem *getCfgInterface(int n) { return this; }

  virtual RootWnd *createWindow(int n, RootWnd *parent);
  virtual void onCreate();
  
  virtual int onCommand(const char *cmd, int param1, int param2, void *ptr, int ptrlen);
  
  void loadConfig();
  void saveConfig();
};

extern WACNAME *shoutcastwac;

#endif
