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
#include "collection.h"


Collection::Collection(const char *_name) {
  name = _name;
  collection = NULL;
}

Collection::~Collection() {
  if (collection != NULL)
    api->service_release(collection);
}

int Collection::addElement(const char *id, XmlReaderParams *params) {
  if (!checkCollection()) return -1;
  return collection->addElement(id, params);
}

void Collection::removeElement(int id) {
  if (!checkCollection()) return;
  collection->removeElement(id);
}

int Collection::getNumElements() {
  if (!checkCollection()) return 0;
  return collection->getNumElements();
}

CollectionElement *Collection::enumElement(int n, int *ancestor) {
  if (!checkCollection()) return NULL;
  return collection->enumElement(n, ancestor);
}

CollectionElement *Collection::getElement(const char *id, int *ancestor) {
  if (!checkCollection()) return NULL;
  return collection->getElement(id, ancestor);
}

int Collection::checkCollection() {
  if (collection == NULL) {
    CollectionSvcEnum cse(name);
    collection = cse.getFirst();
    if (collection == NULL)
      return 0;
  }
  return 1;
}