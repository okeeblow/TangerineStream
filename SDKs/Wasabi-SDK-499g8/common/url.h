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
#ifndef _URL_H
#define _URL_H

#include "common.h"
#include "../bfc/string.h"

#define URLENCODE_NOTHING                 1     // encode nothing (add some flags!), default is to start with everything
#define URLENCODE_EXCLUDEALPHANUM         2     // do not encode characters that are alphanumeric (a-z,0-1) or _ - and .
#define URLENCODE_EXCLUDESLASH		  4	// don't encode '/'
#define URLENCODE_EXCLUDEHTTPPREFIX		  8	// don't encode "http://"
#define URLENCODE_EXCLUDE_8BIT            16     // do not encode > 127
#define URLENCODE_EXCLUDE_7BIT_ABOVEEQ32  32     // do not encode >= 32 & <= 127
#define URLENCODE_ENCODESPACE             64    // force encoding space
#define URLENCODE_ENCODEXML               128    // force encoding '>' and '<' 

#define URLENCODE_DEFAULT                 URLENCODE_EXCLUDEALPHANUM
#define URLENCODE_FULLURL	(URLENCODE_DEFAULT|URLENCODE_EXCLUDEHTTPPREFIX|URLENCODE_EXCLUDESLASH|URLENCODE_EXCLUDE_7BIT_ABOVEEQ32|URLENCODE_ENCODESPACE)

class COMEXP Url {
public:
  static void encode(String &dest, int use_plus_for_space = FALSE, int encoding = URLENCODE_DEFAULT, int style=URLENCODE_STYLE_PERCENT);
  static void decode(String &str, int use_plus_for_space = FALSE);

  enum {
    URLENCODE_STYLE_PERCENT = 0,   // %AB
    URLENCODE_STYLE_ANDPOUND = 1,  // &#171;
    URLENCODE_STYLE_ANDPOUNDX = 2, // &#xAB;
  };
};

#endif
