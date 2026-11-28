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
#ifndef __REENTRYFILTER_H
#define __REENTRYFILTER_H

#include "tlist.h"

using namespace wasabi;

class ReentryFilterObject {
  public:

    ReentryFilterObject() {}
    virtual ~ReentryFilterObject() {}

    void enterScope(int scope) {
      scopes.addItem(scope);
    }

    int isInScope(int scope) {
      for (int i=0;i<scopes.getNumItems();i++)
        if (scopes.enumItem(i) == scope) return 1;
      return 0;
    }

    void leaveScope(int scope) {
      for (int i=0;i<scopes.getNumItems();i++)
        if (scopes.enumItem(i) == scope) {
          scopes.delByPos(i);
          return;
        }
    }

  private:
    
    TList<int> scopes;
};

class ReentryFilter {
  public:

    ReentryFilter() : reentering(0), thisscope(-1), filter(NULL) {} // call setFilterObject, enterScope and leaveScope manually

    ReentryFilter(ReentryFilterObject *filterobj, int scope) {
      setFilterObject(filterobj);
      enterScope(scope);
    }

    void setFilterObject(ReentryFilterObject *filterobj) {
      filter = filterobj;
    }

    void enterScope(int scope) {
      ASSERT(filter != NULL);
      if (filter->isInScope(scope)) {
        reentering = 1;
      } else {
        filter->enterScope(scope);
        reentering = 0;
      }
      thisscope = scope;
    }

    void leaveScope() {
      ASSERT(filter != NULL);
      filter->leaveScope(thisscope);
    }

    virtual ~ReentryFilter() {
      if (reentering) return;
      leaveScope();
    }

    int mustLeave() { return reentering; }

  private:
    int thisscope;
    int reentering;
    ReentryFilterObject *filter;
};

#endif