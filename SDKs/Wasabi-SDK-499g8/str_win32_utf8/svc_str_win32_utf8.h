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
#ifndef _SVC_STR_WIN32_UTF8_H 
#define _SVC_STR_WIN32_UTF8_H 

#include "../studio/services/servicei.h"
#include "../studio/services/svc_stringconverter.h"

//
//  *** Here's our service instantiation for using the IBM ICU for wn32 translation
//
//  Service instantiations, of course, require an implementation of
//  the virtual methods which create the functionality of the service.
//
//  In addition, the use of Service Factories (servicei.h) simplifies
//  the instantiation and use of services themselves.  Because of
//  the template nature of the factories, they require an additional
//  static method to be defined by any service instantiation using a factory.
//
//  Unfortunately, also because of the template nature of the factories,
//  this point isn't terribly obvious.  This is why I am documenting it here.
//
//  Gee, aren't I a swell pal?
//

class svc_str_win32_utf8 : public svc_stringConverterI {
// *** These methods are defined by the actual service we are instantiating.
public:
  // test the type -- return boolean.
  virtual int canConvert(FOURCC encoding_type);

  // alloc the returned memory with api->sysMalloc() -- return NULL on error.
  virtual int convertToUTF8(FOURCC encoding_type, const void *in_buffer, int size_in_bytes, char *out_buffer, int size_out_bytes);
  virtual int preflightToUTF8(FOURCC encoding_type, const void *in_buffer, int size_in_bytes);
  virtual int convertFromUTF8(FOURCC encoding_type, const char *in_buffer, int size_in_bytes, void *out_buffer, int size_out_bytes);
  virtual int preflightFromUTF8(FOURCC encoding_type, const char *in_buffer, int size_in_bytes);

private:
  // Methods only used by this object.
  int convertGetLastError();
  unsigned short * allocCodepageToUTF16(int codepage, const char *in_buffer, int size_in_bytes, int *size_out_bytes);


// *** This static method is required to be defined in order to use a Service Factory.
public:
  static const char *getServiceName() { return "StringConverter for OSNATIVE for win32"; }
};


#endif  // _SVC_STR_WIN32_UTF8_H
