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
#ifndef _XLATSTR_H
#define _XLATSTR_H

#include "../studio/api.h"

/**
  Provides string translation for the string
  used as the constructor parameter.
  
  The constructor will automatically lookup
  the translated value of the string it receives
  in the currently loaded locale.

  @short Translates a string using the currently loaded locale.
  @author Nullsoft
  @ver 1.0
  @see ComponentAPI::locales_getTranslation()
*/
class _ {
public:
  /**
    Automatically looks up the translated value of the string
    it receives as a parameter in the currently loaded
    locale. The same string is returned if there's no
    translation.
    
    @param str String to be translated.
    @ret Translation found, Translated string; Translation not found, Input string;
  */
#if defined(WASABI_COMPILE_LOCALES)
  _(const char *str) { s=WASABI_API_LOCALE->locales_getTranslation(str); }
#else
  _(const char *str) { s=str; }
#endif
  
  operator const char *() const { return s; }
private:
  const char *s;
};

#endif
