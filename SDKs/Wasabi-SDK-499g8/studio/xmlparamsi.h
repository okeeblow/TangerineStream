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
#ifndef __XMLPARAMSI_H
#define __XMLPARAMSI_H

#include "../bfc/dispatch.h"
#include "../bfc/string.h"
#include "../bfc/ptrlist.h"

/*<?<autoheader/>*/
#include "xmlparams.h"
#include "xmlparamsx.h"
/*?>*/

class XmlReaderParamsI : public XmlReaderParamsX {
public:
  XmlReaderParamsI() {}
  virtual ~XmlReaderParamsI();

  DISPATCH(100) const char *getItemName(int i);
  DISPATCH(200) const char *getItemValue(int i);
  DISPATCH(201) const char *getItemValue(const char *name);
  DISPATCH(202) const char *enumItemValues(const char *name, int nb);
  DISPATCH(300) int getItemValueInt(const char *name, int def = 0);
  DISPATCH(400) int getNbItems();

  DISPATCH(500) void addItem(const char *parm, const char *value);
  DISPATCH(600) void removeItem(const char *parm);
  DISPATCH(700) void replaceItem(const char *parm, const char *value);
  DISPATCH(800) int findItem(const char *parm);
private:
  struct parms_struct {
    String parm;
    String value;
  };
  PtrList<parms_struct> parms_list;
};


#endif
