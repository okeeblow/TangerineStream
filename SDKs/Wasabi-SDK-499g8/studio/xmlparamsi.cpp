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
#include "xmlparamsi.h"

XmlReaderParamsI::~XmlReaderParamsI() {
  parms_list.deleteAll();
}

const char *XmlReaderParamsI::getItemName(int i) {
  if(i>getNbItems()) return "";
  return parms_list[i]->parm;
}

const char *XmlReaderParamsI::getItemValue(int i) {
  if(i>getNbItems()) return "";
  return parms_list[i]->value;
}

const char *XmlReaderParamsI::getItemValue(const char *name) {
  for(int i=0;i<getNbItems();i++)
    if(STRCASEEQL(parms_list[i]->parm, name))
      return parms_list[i]->value;
  return NULL;
}

const char *XmlReaderParamsI::enumItemValues(const char *name, int nb) {
  int f=0;
  for(int i=0;i<getNbItems();i++)
    if(STRCASEEQL(parms_list[i]->parm, name))
      if(f==nb)
        return parms_list[i]->value;
      else f++;
  return NULL;
}

int XmlReaderParamsI::getItemValueInt(const char *name, int def) {
  for(int i=0;i<getNbItems();i++)
    if(STRCASEEQL(parms_list[i]->parm, name))
      return atoi(parms_list[i]->value);
  return def;
}

int XmlReaderParamsI::getNbItems() {
  return parms_list.getNumItems();
}

void XmlReaderParamsI::addItem(const char *parm, const char *value) {
  parms_struct *p= new parms_struct;
  p->parm = parm;
  p->value = value;
  parms_list.addItem(p);
}

void XmlReaderParamsI::removeItem(const char *parm) {
  for (int i=0;i<parms_list.getNumItems();i++) {
    parms_struct *s = parms_list.enumItem(i);
    if (STRCASEEQLSAFE(parm, s->parm)) {
      delete s;
      parms_list.removeByPos(i);
      i--;
    }
  }
}

void XmlReaderParamsI::replaceItem(const char *parm, const char *value) {
  String s = value; // if we were passed our current value's pointer ... 
  if (s.isequal(getItemValue(parm))) return; // (hey, if we're replacing with same value, forget about it)

  removeItem(parm); // ... then this call would make the value param invalid ...

  if (!s.isempty())
    addItem(parm, s); // ... so we're sending a saved buffer instead
}

int XmlReaderParamsI::findItem(const char *parm) {
  for(int i=0;i<getNbItems();i++)
    if(STRCASEEQL(parms_list[i]->parm, parm))
      return i;
  return -1;
}

