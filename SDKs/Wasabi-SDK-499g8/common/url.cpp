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
#include "url.h"

#include "../bfc/std.h"

void Url::encode(String &dest, int use_plus_for_space, int encoding, int style) {
  if (dest.isempty()) return;
  String srcstr = dest;
  const char *src = srcstr;
  dest = NULL;
  if (encoding & URLENCODE_EXCLUDEHTTPPREFIX)
    if (!STRNINCMP(src, "http://")) {
      src += STRLEN("http://");
      dest += "http://";
    }
  while (*src) {
    int encode = 1;
    if (encoding & URLENCODE_NOTHING) encode = 0;
    if ((encoding & URLENCODE_EXCLUDEALPHANUM) && (ISALPHA(*src) || ISDIGIT(*src) || *src == '_' || *src == '-' || *src == '.') && (unsigned char)*src < 128) encode = 0;
    if ((encoding & URLENCODE_EXCLUDE_8BIT) && ((unsigned char)*src > 127)) encode = 0;
    if ((encoding & URLENCODE_EXCLUDE_7BIT_ABOVEEQ32) && ((unsigned char)*src >= 32 && (unsigned char)*src <= 127)) encode = 0;
    if ((encoding & URLENCODE_ENCODESPACE) && *src == ' ') encode = 1;
    if ((encoding & URLENCODE_ENCODEXML) && (*src == '<' || *src == '>')) encode = 1;
    if ((*src == '&' && (style == URLENCODE_STYLE_ANDPOUND || style == URLENCODE_STYLE_ANDPOUNDX)) || 
        (*src == '%' && style == URLENCODE_STYLE_PERCENT)) encode = 1;
    if ((encoding & URLENCODE_EXCLUDESLASH) && *src == '/') encode = 0;
    if (!encode) {
      dest += *src;
    } else if (use_plus_for_space && *src == ' ') {
      dest += '+';
    } else {
      switch (style) {
        case URLENCODE_STYLE_PERCENT:
          dest += StringPrintf("%%%02X", (int)*(unsigned char *)src);
          break;
        case URLENCODE_STYLE_ANDPOUND:
          dest += StringPrintf("&#%02d;", (int)*(unsigned char *)src);
          break;
        case URLENCODE_STYLE_ANDPOUNDX:
          dest += StringPrintf("&#x%02X;", (int)*(unsigned char *)src);
          break;
      }
    }
    src++;
  }
}

void Url::decode(String &str, int use_plus_for_space) {
  if (str.isempty()) return;
  char *s = (char *)MALLOC(STRLEN(str)+1);
  char *p = str.getNonConstVal();
  char *d = s;

  while (*p) {
    if (*p == '%') {
      int abort = 0;
      if (!*(p+1)) { *d++ = *p++; break; }
      if (*(p+1) == '%') {
        *d = *p;
        p = p+2;
        d++;
      }
      int a = *(p+1);
      int b = 0;
      if (a != 0) b = *(p+2);
      a = TOUPPER(a);
      b = TOUPPER(b);
      if (a >='0' && a <= '9') a -= '0';
      else if (a >= 'A' && a <= 'F') { a -= 'A'; a += 10; }
      else abort = 1;
      if (b >='0' && b <= '9') b -= '0';
      else if (b >= 'A' && b <= 'F') { b -= 'A'; b += 10; }
      else abort = 1;
      if (abort) {
        *d++ = *p++;
        continue;
      }
      *d++ = a*16+b;
      p = p+3;
      continue;
    }
    *d = *p;
    p++; d++;
  }
  *d = 0;

  str = s;
  FREE(s);
}
