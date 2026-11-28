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
#ifndef _ATTREDIT_H
#define _ATTREDIT_H

#include <bfc/attribs/attribute.h>
#include <bfc/attribs/cfgitem.h>

#include <common/checkwnd.h>
#include <bfc/depend.h>
#include <common/editwnd.h>
#include <bfc/memblock.h>

#define ATTREDIT_PARENT BaseWnd
class AttrEdit : public ATTREDIT_PARENT, public DependentViewerI {
protected:
  AttrEdit(CfgItem *item=NULL, const char *name=NULL);

public:
  virtual int onPaint(Canvas *canvas);	// from BaseWnd

  void setAttr(CfgItem *item, const char *name);
  const char *getAttributeName();

  static AttrEdit *create(CfgItem *item, const char *name);

protected:
  virtual int viewer_onItemAttrChange(Dependent *item, const char *attr);
  virtual void dataChanged();

  CfgItem *cfgitem;
  String attrname;
};

#define ATTREDITBOOL_PARENT AttrEdit
class AttrEditBool : public ATTREDITBOOL_PARENT {
public:
  AttrEditBool(CfgItem *item=NULL, const char *name=NULL);
  virtual ~AttrEditBool();

protected:
  virtual int onInit();
  virtual int onResize();

  virtual int childNotify(RootWnd *child, int msg, int param1=0, int param2=0);

  virtual void dataChanged();

private:
  CheckWnd *checkwnd;
};

#define ATTREDIT_TEXT_PARENT AttrEdit
class AttrEdit_text : public ATTREDIT_TEXT_PARENT {
public:
  AttrEdit_text(CfgItem *item, const char *name);
  virtual ~AttrEdit_text();

protected:
  virtual int onResize();

  virtual void onLeftPush(int x, int y);		// from ClickWnd

  virtual int convertToString(MemBlock<char> &editbuf)=0;
//  virtual int onEditDone(const char *str)=0;

private:
  EditWnd *editwnd;
  MemBlock<char> editbuf;
};

class AttrEditInt : public AttrEdit_text {
public:
  AttrEditInt(CfgItem *item, const char *name) : AttrEdit_text(item, name) {}

protected:
  virtual int convertToString(MemBlock<char> &editbuf);
//  virtual void onEditDone(const char *str);
};

class AttrEditString : public AttrEdit_text {
public:
  AttrEditString(CfgItem *item, const char *name) : AttrEdit_text(item, name) {}

protected:
  virtual int convertToString(MemBlock<char> &editbuf);
};

#include <bfc/wnds/form/formwnd.h>

class AttrEditCell : public FormCell {
public:
  AttrEditCell(AttrEdit *ae=NULL) {
    setAttrEdit(ae);
  }

  void setAttrEdit(AttrEdit *ae) {
    setContent(attredit = ae);
  }

private:
  AttrEdit *attredit;
};

#endif
