/*

  Nullsoft WASABI Source File License

  Copyright 1999-2001 Nullsoft, Inc.

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


  Brennan Underwood
  brennan@nullsoft.com

*/

#ifndef __AUTOQUERYLIST_H
#define __AUTOQUERYLIST_H

#include "itemlistwnd.h"
#include "../db/multiqueryserver.h"
#include "../ptrlist.h"
#include "../string.h"
#include "../timeslicer.h"
#include "../appcmds.h"
#include "../../pledit/plhand.h"
#include "../nakedobject.h"

class svc_plDir;
class Playlist;

/**
  class AutoQueryList 
  
  @short 
  @author Nullsoft
  @ver 1.0
  @see 
  @cat BFC
*/
#define AUTOQUERYLIST_PARENT NakedObject
#define AUTOQUERYLIST_DBPARENTSRV MultiQueryServerI
class AutoQueryList : public AUTOQUERYLIST_PARENT, 
                      public AUTOQUERYLIST_DBPARENTSRV, AppCmdsI {

  public:
  AutoQueryList();
  virtual ~AutoQueryList();

  virtual int onInit();

  virtual void mqs_onAddPlaystring(const char *playstring, int nitems, int thispos);
  virtual void mqs_onNewMultiQuery();
  virtual void mqs_onCompleteMultiQuery();

protected:
  virtual void appcmds_onCommand(int id, const RECT *buttonRect);

private:

  int need_startslicer;
  int lastpc;
  int nfound;
  Playlist *playlist;
  svc_plDir *pldir;
};

#endif
