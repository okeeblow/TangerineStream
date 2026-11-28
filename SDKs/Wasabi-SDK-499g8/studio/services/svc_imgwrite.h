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
#ifndef _SVC_IMGWRITE_H
#define _SVC_IMGWRITE_H

#include <bfc/std.h>
#include <bfc/dispatch.h>
#include <bfc/parse/pathparse.h>

class NOVTABLE svc_imageWriter /*: public Dispatchable*/ {
public:
  static FOURCC getServiceType() { return WaSvc::IMAGEWRITER; }

  virtual const char *getImageTypeName()=0;
  virtual const char *getExtensions()=0;	// "jpg;jpeg" etc

  virtual int isLossless()=0;
  virtual int setBasicQuality(int q)=0;	// 0..100

  // free the returned buffer with sysFree()
  virtual unsigned char *convert(const ARGB32 *pixels, int w, int h, int *length)=0;
};

class svc_imageWriterI : public svc_imageWriter {
public:
  virtual const char *getImageTypeName()=0;
  virtual int isLossless()=0;
  virtual int setBasicQuality(int q)=0;	// 0..100
  virtual unsigned char *convert(const ARGB32 *pixels, int w, int h, int *length)=0;
};

#include <bfc/svc_enum.h>

class ImgWriterEnum : public SvcEnumT<svc_imageWriter> {
public:
  ImgWriterEnum(const char *_ext=NULL) : ext(_ext) { }

protected:
  virtual int testService(svc_imageWriter *svc) {
    if (ext.isempty()) return 1;
    else {
      PathParser pp(svc->getExtensions(), ";");
      for (int i = 0; i < pp.getNumStrings(); i++)
        if (STRCASEEQL(ext, pp.enumString(i))) return 1;
      return 0;
    }
  }
  String ext;
};

#endif
