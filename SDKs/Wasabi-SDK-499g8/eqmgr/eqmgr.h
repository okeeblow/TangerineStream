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
#ifndef _EQMGR_H
#define _EQMGR_H

#include <studio/wac.h>
#include <core/corecb.h>

class WACEqMgr : public WAComponentClient, public CoreCallbackI {
public:
  WACEqMgr();

  virtual const char *getName() { return "EQ Manager"; }
  virtual GUID getGUID();

  virtual void onCreate();
  virtual void onDestroy();
  virtual int onCommand(const char *cmd, int param1, int param2, void *ptr, int ptrlen);

  void doPresetsMenu();
  int readEQfile(const char *file, const char *name, int apply_preamp=TRUE);
  int writeEQfile(const char *file, const char *name);
  void deleteEQfile(const char *file, const char *name);
  
  void doAutoEq(const char *playstring);

  virtual int corecb_onSetNextFile(const char *playstring);

private:

  void loadDefaultPreset();
  void saveDefaultPreset();
  void loadFromEQF();
  void saveToEQF();
  void loadPresetDialog();
  void savePresetDialog();
  void loadAutoPresetDialog();
  void saveAutoPresetDialog();
  void deletePresetDialog();
  void deleteAutoPresetDialog();
  
  int writeEQfile_init(const char *file, char *name, unsigned char *tab);
};

#endif
