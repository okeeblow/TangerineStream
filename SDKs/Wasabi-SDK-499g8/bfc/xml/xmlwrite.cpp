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

#include "xmlwrite.h"

#include <bfc/std.h>
#include <common/utf8.h>
#include <bfc/string.h>
#include <bfc/paramparser.h>

#define XMLVERSION "1.0"

#include <studio/xml/xmltok_impl.h>

static unsigned char xmltypecheck[256] = {
#define BT_COLON BT_NMSTRT
#include <studio/xml/asciitab.h>
#undef BT_COLON
#include <studio/xml/latin1tab.h>
};

#define EFPRINTF (nohicharconversion ? fprintf : efprintf)

XMLWrite::XMLWrite(const char *filename, const char *doctype, const char *dtddoctype, int no_hi_chars_conversion) {
  nohicharconversion = no_hi_chars_conversion;
  Init(fopen(filename, "wt"), doctype, dtddoctype);
}

XMLWrite::XMLWrite(FILE *file, const char *doctype, const char *dtddoctype, int no_hi_chars_conversion) {
  nohicharconversion = no_hi_chars_conversion;
  Init(file,doctype, dtddoctype);
}

void XMLWrite::Init(FILE *file, const char *doctype, const char *dtddoctype) {
  fp = file;
  ASSERT(fp != NULL);   // sheet, need exceptions here
  indent = 0;
  fprintf(fp, "<?xml version=\"" XMLVERSION "\" encoding='UTF-8' standalone=\"yes\"?>\n");
  if (dtddoctype != NULL) fprintf(fp, "<!DOCTYPE %s>\n", dtddoctype);
  pushCategory(doctype, 1, 0);
}

XMLWrite::~XMLWrite() {
  popCategory(1, 0);
  fflush(fp);
  fclose(fp);
  ASSERT(titles.peek() == 0);
}

void XMLWrite::comment(const char *comment) {
  fprintf(fp, "<!-- %s -->\n", comment);
}

void XMLWrite::pushCategory(const char *title, int wantcr, int wantindent) {
  if (wantindent) // because %*s will put one space event if ident == 0 :/
    fprintf(fp, "%*s<%s>%s", indent*2, " ", title, wantcr ? "\n": "");
  else
    fprintf(fp, "<%s>%s", title, wantcr ? "\n": "");
  indent++;
  ParamParser pp(title, " ");
  titles.push(STRDUP((char *)pp.enumItem(0)));
}

void XMLWrite::pushCategoryAttrib(const char *title, int nodata) {
  fprintf(fp, "%*s<%s", indent*2, " ", title);
  indent++;
  titles.push(nodata ? NULL : STRDUP((char *)title));
}

void XMLWrite::writeCategoryAttrib(const char *title, const int val) {
  fprintf(fp, " %s=\"%d\"", title, val);
}

void XMLWrite::writeCategoryAttrib(const char *title, const char *str) {
  fprintf(fp, " %s=\"", title);
  EFPRINTF(fp, "%s", str);
  fprintf(fp, "\"");
}

void XMLWrite::closeCategoryAttrib(int wantcr) {
  if (titles.top() == NULL) fprintf(fp, " /");
  fprintf(fp, ">%s", wantcr ? "\n" : "");
}

void XMLWrite::writeAttrib(const char *title, const char *text, int wantcr, int wantindent) {
  if (wantindent)
    fprintf(fp, "%*s<%s>", indent*2, " ", title);
  else
    fprintf(fp, "<%s>", title);
  EFPRINTF(fp, "%s", text);
  fprintf(fp, "</%s>%s", title, wantcr ? "\n" : "");
}

void XMLWrite::writeAttrib(const char *title, int val, int wantcr, int wantindent) {
  if (wantindent)
    fprintf(fp, "%*s<%s>%d</%s>%s", indent*2, " ", title, val, title, wantcr ? "\n" : "");
  else
    fprintf(fp, "<%s>%d</%s>%s", title, val, title, wantcr ? "\n" : "");
}

int XMLWrite::popCategory(int wantcr, int wantindent) {
  indent--;
  char *title;
  int r = titles.pop(&title);
  if (!r) return 0;
  if (title != NULL) {
    if (wantindent)
      fprintf(fp, "%*s</%s>%s", indent*2, " ", title, wantcr ? "\n" : "");
    else
      fprintf(fp, "</%s>%s", title, wantcr ? "\n" : "");
    FREE(title);
  }
  return titles.peek();
}

int XMLWrite::efprintf(FILE *fp, const char *format, ...) {
  va_list v;
// http://www.w3.org/TR/REC-xml#syntax
  int bcount=0;
  String outstr;
  va_start(v, format);
  outstr.vsprintf(format, v);
  va_end(v);
  int n = outstr.len();
  for (int i = 0; i < n; i++) {
    unsigned int c = ((unsigned char *)outstr.getValue())[i];
    switch (c) {
      case '<': fputs("&lt;", fp); bcount += 4; break;
      case '>': fputs("&gt;", fp); bcount += 4; break;
      case '&': fputs("&amp;", fp); bcount += 5; break;
      case '\"': fputs("&quot;", fp); bcount += 6; break;
      case '\'': fputs("&apos;", fp); bcount += 6; break;
      default:
        if (xmltypecheck[c] != 0) {
          if (c >= 127)	{ // ASCII-centric, sorry
            fprintf(fp, "&#x%02x;", c);
            bcount += 6;
          } else {
            fputc(c, fp);
            bcount++;
          }
        }
      break;
    }
  }
  return bcount;
}

static void unescape(char *str, char *esc, char replace) {
  ASSERT(str != NULL);
  if (*str == 0) return;	// ignore empty strings
  char *c;
  ASSERT(esc != NULL);
  int slen = STRLEN(esc);
  ASSERT(slen > 0);
  while ((c = STRSTR(str, esc)) != NULL) {
    *c = replace;
    if (c[slen-1]) STRCPY(c+1, c+slen);
    else c[1] = 0;
  }
}

void xmlUnescape(char *str) {
  unescape(str, "&lt;", '<');
  unescape(str, "&gt;", '>');
  unescape(str, "&amp;", '&');
  unescape(str, "&quot;", '\"');
  unescape(str, "&apos;", '\'');
  String dup = str;
  UTF8_to_ASCII(dup, str);
}
