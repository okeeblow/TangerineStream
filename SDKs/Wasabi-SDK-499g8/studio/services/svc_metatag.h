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
#ifndef _SVC_METATAG_H
#define _SVC_METATAG_H

//UNDER CONSTRUCTION

#include "../../bfc/dispatch.h"
#include "services.h"

class svc_metaTag : public Dispatchable {
public:
  static FOURCC getServiceType() { return WaSvc::METATAG; }

  const char *getName();	// i.e. "ID3v2" or something
  
  int testFile(const char *filename);	// or URL, or any playstring

  const char *enumSupportedTag(int n, int *datatype=NULL);	// see metatag.h

  int getMetaData(const char *tag, char *buf, int buflen, int datatype=0);
  int setMetaData(const char *tag, const char *buf, int buflen, int datatype=0);
};

#endif
