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
#include "watcher.h"

#include "../studio/api.h"
#include "../common/thread.h"
#include "../common/critsec.h"
#include "../bfc/util/recursedir.h"
#include "../common/playstring.h"
#include "../bfc/textfeed.h"

#define FEEDID "library.addmusic.status"

class MyTextFeed : public TextFeed {
public:
  MyTextFeed() {
    registerFeed(FEEDID);
  }
  static const char *getServiceName() { return "honk"; }
};

static waServiceTSingle<svc_textFeed, MyTextFeed> feedo;
waServiceFactoryI *feedolio = &feedo;

static void sendo(const char *txt) {
  feedo.getSingleService()->sendFeed(FEEDID, txt);
}

//CUT #define TIMER_NEXT 10

#define MAX_ENQUEUE 1000

class WatcherThread : public Thread {
public:
  WatcherThread() : Thread(WASABI_API_APP->main_getMainThreadHandle()) { }
  virtual int threadProc() {
    while (!getKillSwitch())
      if (!Watcher::threadProc()) break;
    return 1;
  }
};

static CriticalSection dircs;
static PtrList<RecurseDir> dirs;

static CriticalSection filecs;
static PtrList<Playstring> files;

static Watcher *watcher;
static WatcherThread *watcher_thread;
static int watcher_idle = 1;

static CriticalSection extlistcs;
static char *extlist;

Watcher::Watcher() {
  timer_set = 0;
}

void Watcher::onSlice() {
  INCRITICALSECTION(filecs);
  if (files.getNumItems() == 0) {
    if (watcher_idle) stopSlicer();
    return;
  }

  // cram into the db
  Playstring *fn = files.q(0);
  api->metadb_addNewItem(fn->getValue());

sendo(fn->getValue());

  delete fn;
  files.removeByPos(0);
}

void Watcher::addDir(const char *dir) {
  if (!Std::isDirectory(dir)) return;
  if (watcher == NULL) watcher = new Watcher;
  extlistcs.enter();
  FREE(extlist);
  const char *p = api->core_getSupportedExtensions();
  for (const char *x = p; ; x++) {
    if (x[0] == '\0' && x[1] == '\0') break;
  }
  int l = (x - p) + 2;
  extlist = (char *)MEMDUP(p, l);
  extlistcs.leave();
  watcher->_addDir(dir);
}

void Watcher::_addDir(const char *dir) {
  INCRITICALSECTION(dircs);
  foreach(dirs)
    if (PATHEQL(dir, dirs.getfor()->getOriginalPath())) return;
  endfor
  dirs.addItem(new RecurseDir(dir));
  startSlicer();
//  if (!timer_set) {
//    timerclient_setTimer(TIMER_NEXT, 100);
//    timer_set = 1;
    if (watcher_thread == NULL) {
      watcher_thread = new WatcherThread;
      watcher_thread->start();
    }
//  }
}
int checkExt(const char *filename) {
  ASSERT(extlist != NULL);
  ASSERT(filename != NULL);
  const char *ext = Std::extension(filename);
  if (ext == NULL) return 0;

  const char *e = extlist;
  for (;;) {
    if (*e == 0) break;
    if (STRCASEEQL(ext, e)) return 1;
    e += STRLEN(e)+1;
  }
  return 0;
}

int Watcher::threadProc() {
  filecs.enter();
  int nf = files.getNumItems();
  filecs.leave();

  dircs.enter();
  int n = dirs.getNumItems();
  dircs.leave();
  
  if (!n) {
    watcher_idle = 1;
    return 1;
  }

  if (nf >= MAX_ENQUEUE) {
    Std::usleep(1000);
    return 1;
  }

  watcher_idle = 0;

  INCRITICALSECTION(dircs);
  RecurseDir *rd = dirs.q(0);
  if (!rd->next()) {
    // done eh
    delete rd;
    dirs.removeByPos(0);

    if (dirs.getNumItems() == 0) {
      FREE(extlist); extlist = NULL;
    }

    return 1;	// come back around and check for more
  }
  if (checkExt(rd->getFilename())) {
    INCRITICALSECTION(filecs);
    files.addItem(new Playstring(StringPrintf("file:%s%s%s", rd->getPath(), DIRCHARSTR, rd->getFilename())));
//CUT OutputDebugString("sending from thread: ");
//CUT OutputDebugString(rd->getFilename());
  }
  Std::usleep(0);

  return 1;
}

