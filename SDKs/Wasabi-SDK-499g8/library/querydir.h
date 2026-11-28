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
#ifndef _QUERYDIR_H
#define _QUERYDIR_H

#include "../common/objectdir.h"
#include "../common/filename.h"
#include "../bfc/timerclient.h"
#include "../studio/runlevelcb.h"

class Query {
public:
  Query(const char *qt="") : querytext(qt) { }

  FilenameI querytext;
};

#define QUERYDIR_PARENT ObjectDirT<Query>
class QueryDir : public QUERYDIR_PARENT {
public:
  QueryDir();
  virtual ~QueryDir();
  static const char *getServiceName() { return "Query Directory"; }

  virtual int onAction(int action, RootWnd *from, const char *target, ObjectHandle handle);

  virtual Dependent *timerclient_getDependencyPtr() { return this; }
//  virtual int timerclient_onDeferredCallback(int param1, int param2);
  virtual void runlevelcb_onStartup();

private:
  ObjectHandle makeAutoQuery(const char *family);
};

#endif
