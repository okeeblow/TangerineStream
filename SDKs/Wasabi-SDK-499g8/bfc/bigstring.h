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
#ifndef __BIGSTRING_H
#define __BIGSTRING_H

/*

A class tor concatenate chunks of texts into one big pool. This is much faster than using String if you are adding
a lot of tiny pieces into one giant block (a typical cases of this is when writing xml). Upon request for the value,
the class allocates one big block of memory and copies all the strings into it serially (as opposed to String 
reallocating the entire block at each concatenation). Note that because of the type of implementation BigString has 
to use, you cannot get the full block as a return value to your concatenations and assignments (+= and = return void). 
To do this, request the value explicitely (this should be kept to a strict minimum or the advantage of BigString over
String will disapear)

*/

#include "bfc/string.h"
#include "bfc/ptrlist.h"

class BigString {
  public:
    BigString() {
      mem = NULL;
      m_linecount = 0;
    }
    virtual ~BigString() {
      if (mem != NULL) {
        FREE(mem);
      }
      strings.deleteAll();
    }

    operator const char *() const { return getValue(); }
    
    const char *getValue() const {
      if (mem != NULL) return mem;
      int l = 0;
      foreach(strings)
        l += strings.getfor()->len();
      endfor;
      (char *)mem = (char *)MALLOC(l+1);
      char *p = mem;
      String *s = NULL;
      int sl = 0;
      foreach(strings)
        s = strings.getfor();
        sl = s->len();
        if (sl > 0) MEMCPY((void *)p, (void *)s->getValue(), sl);
        p += sl;
      endfor;
      *p = 0;
      return mem;
    }

    char *getNonConstVal() { return (char *)getValue(); }

    void setValue(const char *val) {
      if (mem != NULL) {
        FREE(mem);
        mem = NULL;
      }
      strings.deleteAll();
      cat(val);
    }
    // copy assignment operator
    BigString& operator =(const BigString &s) {
      if (this != &s)
        setValue(s);
      return *this;
    }

    void operator =(const char *newval) { setValue(newval); }
    void operator +=(const char *addval) {
      cat(addval);
    }

    int isempty() {
      if (strings.getNumItems() == 0) return 1;
      foreach(strings)
        if (!strings.getfor()->isempty()) return 0;
      endfor;
      return 1;
    }

    void reset() {
      if (mem != NULL) {
        FREE(mem);
        mem = NULL;
      }
      strings.deleteAll();
      m_linecount = 0;
    }

    void catn(const char *s, int n) {
      String *str = new String();
      str->catn(s, n);
      cat(str->getValue());
    }

    void cat(const char *s) {
      if (mem != NULL) {
        FREE(mem);
        mem = NULL;
      }
      char *p = (char *)s;
      while (p && *p) {
        if (*p == '\r' || *p == '\n') {
          if (*(p+1) == '\n' && *p == '\r') p++;
          m_linecount++;
        }
        p++;
      }
      strings.addItem(new String(s));
    }

    char lastChar() {
      return strings.getLast()->lastChar();
    }

    char firstChar() {
      const char *s = strings.getFirst()->getValue();
      return s ? *s : 0;
    }

    int getLineCount() {
      return m_linecount;
    }

  private:
    PtrList<String> strings;
    char *mem;
    int m_linecount;
};

#endif
