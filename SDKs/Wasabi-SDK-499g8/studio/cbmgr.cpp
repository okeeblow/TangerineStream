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
#include "cbmgr.h"
#include "syscb.h"
#include "wac.h"

#include "../common/multimap.h"

static MultiMap<int, SysCallback> cblist;
static PtrList<SysCallback> delete_list;
static int reentry_counter=0;

void CallbackManager::registerCallback(SysCallback *cb, void *param, WaComponent *owner) {
  cblist.multiAddItem(cb->getEventType(), cb, TRUE);
}

void CallbackManager::deregisterCallback(SysCallback *cb, WaComponent *owner) {
  delete_list.addItem(cb);
  cblist.multiDelItem(cb->getEventType(), cb);	// remove ref
}

void CallbackManager::issueCallback(int eventtype, int msg, int param1, int param2) {
  ASSERT(reentry_counter >= 0);
  reentry_counter++;
  const PtrList<SysCallback> *mlist = cblist.getListForIndex(eventtype);
  if (mlist != NULL) {
    foreach(mlist)
      SysCallback *cb = mlist->getfor();
      ASSERT(cb != NULL);
      if (!delete_list.haveItem(cb))
        cb->notify(msg, param1, param2);
    endfor
  }
  reentry_counter--;
  if (reentry_counter == 0)
    delete_list.removeAll();
}

int CallbackManager::getNumCallbacks() {
  return cblist.getNumItems();
}
