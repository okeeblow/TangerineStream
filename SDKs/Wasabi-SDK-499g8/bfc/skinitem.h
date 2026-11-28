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
#ifndef __SKINITEM_H
#define __SKINITEM_H

#include "dispatch.h"

class XmlReaderParams;

/*--------------------------------
SkinItem
  const char *getIncludePath();
  const char *getName();
  XmlReaderParams *getParams();
  int getSkinPartId();
--------------------------------*/

class SkinItem : public Dispatchable {
  public:
    const char *getXmlRootPath();
    const char *getName();
    XmlReaderParams *getParams();
    int getSkinPartId();
    SkinItem *getAncestor();

  enum {
    SKINITEM_GETXMLROOTPATH = 0,
    SKINITEM_GETNAME = 10,
    SKINITEM_GETPARAMS = 20,
    SKINITEM_GETSKINPARTID = 30,
    SKINITEM_GETANCESTOR = 40,
  };
};

inline const char *SkinItem::getXmlRootPath() {
  return _call(SKINITEM_GETXMLROOTPATH, (const char *)0);
}

inline const char *SkinItem::getName() {
  return _call(SKINITEM_GETNAME, (const char *)0);
}

inline XmlReaderParams *SkinItem::getParams() {
  return _call(SKINITEM_GETPARAMS, (XmlReaderParams *)NULL);
}

inline int SkinItem::getSkinPartId() {
  return _call(SKINITEM_GETSKINPARTID, (int)0);
}

inline SkinItem *SkinItem::getAncestor() {
  return _call(SKINITEM_GETANCESTOR, (SkinItem *)NULL);
}

class SkinItemI : public SkinItem {
  public:
    virtual const char *getXmlRootPath()=0;
    virtual const char *getName()=0;
    virtual XmlReaderParams *getParams()=0;
    virtual int getSkinPartId()=0;
    virtual SkinItem *getAncestor()=0;

  protected:
    RECVS_DISPATCH;
};

#endif
