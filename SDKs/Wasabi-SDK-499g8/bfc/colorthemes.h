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
#ifndef __COLORTHEMEGROUP_H
#define __COLORTHEMEGROUP_H

#include "dispatch.h"

class ColorThemeGroup : public Dispatchable {
  public:
    const char *getName();
    int getRed();
    int getGreen();
    int getBlue();
    int getGray();
    int getBoost();

    void setName(const char *name);
    void setRed(int r);
    void setGreen(int g);
    void setBlue(int b);
    void setGray(int g);
    void setBoost(int b);

  enum {
    COLORTHEMEGROUPGETNAME=10,
    COLORTHEMEGROUPGETRED=20,
    COLORTHEMEGROUPGETGREEN=30,
    COLORTHEMEGROUPGETBLUE=40,
    COLORTHEMEGROUPGETGRAY=50,
    COLORTHEMEGROUPGETBOOST=60,
    COLORTHEMEGROUPSETNAME=70,
    COLORTHEMEGROUPSETRED=80,
    COLORTHEMEGROUPSETGREEN=90,
    COLORTHEMEGROUPSETBLUE=100,
    COLORTHEMEGROUPSETGRAY=110,
    COLORTHEMEGROUPSETBOOST=120,
  };
};

inline const char *ColorThemeGroup::getName() {
  return _call(COLORTHEMEGROUPGETNAME, (const char *)NULL);
}

inline int ColorThemeGroup::getRed() {
  return _call(COLORTHEMEGROUPGETRED, 0);
}

inline int ColorThemeGroup::getGreen() {
  return _call(COLORTHEMEGROUPGETGREEN, 0);
}

inline int ColorThemeGroup::getBlue() {
  return _call(COLORTHEMEGROUPGETBLUE, 0);
}

inline int ColorThemeGroup::getGray() {
  return _call(COLORTHEMEGROUPGETGRAY, 0);
}

inline int ColorThemeGroup::getBoost() {
  return _call(COLORTHEMEGROUPGETBOOST, 0);
}

inline void ColorThemeGroup::setName(const char *name) {
  _voidcall(COLORTHEMEGROUPSETNAME, name);
}

inline void ColorThemeGroup::setRed(int r) {
  _voidcall(COLORTHEMEGROUPSETRED, r);
}

inline void ColorThemeGroup::setGreen(int g) {
  _voidcall(COLORTHEMEGROUPSETGREEN, g);
}

inline void ColorThemeGroup::setBlue(int b) {
  _voidcall(COLORTHEMEGROUPSETBLUE, b);
}

inline void ColorThemeGroup::setGray(int g) {
  _voidcall(COLORTHEMEGROUPSETGRAY, g);
}

inline void ColorThemeGroup::setBoost(int b) {
  _voidcall(COLORTHEMEGROUPSETBOOST, b);
}

#endif
