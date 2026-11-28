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
#ifndef __WA2XML_H
#define __WA2XML_H

#include "../bfc/ptrlist.h"
#include "../bfc/string.h"
#include "../bfc/memblock.h"
#include "../studio/services/svc_xmlprov.h"

class Wa2XmlProvider : public svc_xmlProviderI {
public:
  // service
  static const char *getServiceName() { return "Winamp 2.x Xml Provider"; }

  int testDesc(const char *desc);
  const char *getXmlData(const char *desc, const char *incpath, XmlReaderParams *params=NULL);

private:
  String xml;
};

#endif
