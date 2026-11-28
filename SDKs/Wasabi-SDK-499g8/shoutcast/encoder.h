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
#ifndef _ENCODER_H
#define _ENCODER_H

#include "shoutcastwnd.h"
#include "../studio/services/svc_mediaconverter.h"

//#define USEOGG

class Encoder: public svc_mediaConverterI {
public:
  virtual ~Encoder() { }

  virtual void initConfig(ContWnd *configwnd, int x, int y) { }
  virtual void updateConfig() { }
  virtual void getConfig() { }
  virtual void loadConfig(const char *pre) { }
  virtual void saveConfig(const char *pre) { }
  virtual void removeConfig() { }

  virtual const char *getContentType() { return ""; }
};

class Encoders {
public:
  static const char *enumNames(int n);
  static Encoder *createInstance(int n);
};

#endif