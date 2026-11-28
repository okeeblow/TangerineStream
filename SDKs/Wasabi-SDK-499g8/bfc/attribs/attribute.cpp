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

#include "attribute.h"
#include "cfgitemi.h"

#include <studio/api.h>

Attribute::Attribute(const char *newname, const char *_desc) :
  Named(newname), desc(_desc), cfgitemi(NULL), private_storage(NULL) { }

Attribute::~Attribute() {
  delete private_storage;
}

const char *Attribute::getAttributeName() {
  return Named::getName();
}

const char *Attribute::getAttributeDesc() {
  return desc;
}

int Attribute::getValueAsInt() {
  char buf[1024]="";
  getData(buf, sizeof buf);
  return ATOI(buf);
}

int Attribute::setValueAsInt(int newval, bool def) {
  return setData(StringPrintf(newval), def);
}

double Attribute::getValueAsDouble() {
  char buf[1024];
  getData(buf, sizeof buf);
  return ATOF(buf);
}

double Attribute::setValueAsDouble(double newval, bool def) {
  return setData(StringPrintf(newval), def);
}

int Attribute::getDataLen() {
  if (private_storage != NULL)
    return private_storage->len()+1;

  ASSERTPR(WASABI_API_CONFIG != NULL, "getDataLen() before API");
  int r = WASABI_API_CONFIG->getStringPrivateLen(mkTag());
  if (r < 0) {
    r = default_val.len()+1;
  }
  return r;
}

int Attribute::getData(char *data, int data_len) {
  if (data == NULL || data_len < 0) return 0;
  if (private_storage != NULL) {
    if (private_storage->isempty()) {
      if (data_len >= 1) {
        *data = 0;
        return 1;
      }
      return 0;
    }
    STRNCPY(data, private_storage->getValue(), data_len);
    return MIN(private_storage->len(), data_len);
  }
  ASSERTPR(WASABI_API_CONFIG != NULL, "can't get without api");
  if (WASABI_API_CONFIG == NULL) return 0;
//  StringPrintf s("fetching %s, dl %d, defval %s\n", getName(), data_len, default_val.getValue());
//  DebugString(s);
  int r = WASABI_API_CONFIG->getStringPrivate(mkTag(), data, data_len, default_val.isempty() ? "" : default_val.getValue());
//  StringPrintf s2("ret '%s'\n", data);
//  DebugString(s2);
  return r;
}

int Attribute::setData(const char *data, bool def) {
  if (def) {	// setting default value
    default_val = data;
    return 1;
  }
  ASSERTPR(WASABI_API_CONFIG != NULL, "can't set data before api");
  if (WASABI_API_CONFIG == NULL) return 0;

  int r = setDataNoCB(data);
  if (r && cfgitemi != NULL) cfgitemi->cfgitem_onAttribSetValue(this);
  return r;
}

int Attribute::setDataNoCB(const char *data) {
  if (private_storage != NULL) {
    private_storage->setValue(data);
  } else {
    WASABI_API_CONFIG->setStringPrivate(mkTag(), data);
  }
  dependent_sendEvent(Attribute::depend_getClassGuid(), Event_DATACHANGE);
  return 1;
}

void Attribute::setCfgItem(CfgItemI *item) {
  delete private_storage; private_storage = NULL;
  ASSERT(cfgitemi == NULL || item == NULL);
  cfgitemi = item;
  if (cfgitemi != NULL) {
    if (cfgitemi->cfgitem_usePrivateStorage()) private_storage = new String;
  }
}

String Attribute::mkTag() {
  String ret(cfgitemi ? cfgitemi->cfgitem_getPrefix() : NULL);
  ret.cat(getName());
  return ret;
}

void Attribute::disconnect() {
  setCfgItem(NULL);
}
