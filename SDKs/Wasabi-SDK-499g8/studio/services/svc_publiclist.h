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
#ifndef __SVCPUBLICLIST_H
#define __SVCPUBLICLIST_H

#include "dispatch.h"
#include "../common/common.h"
#include "string.h"
#include "ptrlist.h"

class DragItem;

class COMEXP ContextCmds : public Dispatchable {
public:

  int contextcmds_getNumCmds();

  const char *contextcmds_enumCmd(int n, int *side, int *id);

  enum {
    SIDE_LEFT=0, SIDE_RIGHT=1
  };

  void contextcmds_onCommand(int id);
  Dependent *contextcmds_getDependencyPtr();

  enum {
    CONTEXTCMDS_GETNUMCMDS=10,
    CONTEXTCMDS_ENUMCMD=20,
    CONTEXTCMDS_ONCOMMAND=30,
    CONTEXTCMDS_GETDEPENDENCYPTR=40,
  };
};

inline int ContextCmds::contextcmds_getNumCmds() {
  return _call(CONTEXTCMDS_GETNUMCMDS, 0);
}

inline const char *ContextCmds::contextcmds_enumCmd(int n, int *side, int *id) {
  return _call(CONTEXTCMDS_ENUMCMD, (const char *)NULL, n, side, id);
}

inline void ContextCmds::contextcmds_onCommand(int id) {
  _voidcall(CONTEXTCMDS_ONCOMMAND, id);
}

inline Dependent *ContextCmds::contextcmds_getDependencyPtr() {
  return _call(CONTEXTCMDS_GETDEPENDENCYPTR, (Dependent *)NULL);
}

class CmdRec {
public:
  CmdRec(const char *name, int _id, int _side) : cmdname(name), id(_id), side(_side) {}
  virtual ~CmdRec() {}
  String cmdname;
  int id, side;

  virtual void onCommand() {}
};

class COMEXP ContextCmdsI : public ContextCmds, public DependentI {
protected:
  void contextcmds_addCmd(CmdRec *cmdrec);
  void contextcmds_addCmd(const char *name, int id, int side=SIDE_LEFT);

public:
  virtual int contextcmds_getNumCmds();
  virtual const char *contextcmds_enumCmd(int n, int *side, int *id);

  // override this and catch your commands, otherwise it'll call the CmdRec
  virtual void contextcmds_onCommand(int id);
  virtual Dependent *contextcmds_getDependencyPtr() { return this; }

protected:
  RECVS_DISPATCH;
  PtrList<CmdRec> cmds;
};

class ContextCmdRecFromItem : public CmdRec {
public:
  ContextCmdRecFromItem(void *item, const char *itemtype, const char *name);

  virtual void onCommand();

private:
  void *item;
  String datatype;
};

#endif


#endif