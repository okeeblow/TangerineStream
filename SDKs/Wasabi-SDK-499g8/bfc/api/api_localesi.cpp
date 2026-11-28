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
//<?#include "<class data="implementationheader"/>"
#include "api_localesi.h"
//?>

#include "studio/localesmgr.h"
#include "studio/keyboard.h"

api_locales *localesApi = NULL;

api_localesI::api_localesI() {
  LocalesManager::init();
}

api_localesI::~api_localesI() {
}

const char *api_localesI::locales_getTranslation(const char *str) {
  return LocalesManager::getTranslation(str);
}

void api_localesI::locales_addTranslation(const char *from, const char *to) {
  LocalesManager::addTranslation(from, to);
}

const char *api_localesI::locales_getBindFromAction(int action) {
  return LocalesManager::getBindFromAction(action);
}

int api_localesI::locales_getNumEntries() {
  return LocalesManager::getNumLocales();
}

const char *api_localesI::locales_enumEntry(int n) {
  return LocalesManager::enumLoadableLocales(n);
}

void api_localesI::locales_registerAcceleratorSection(const char *name, RootWnd *wnd, int global) {
  Keyboard::registerAcceleratorSection(name, wnd, global);
}
