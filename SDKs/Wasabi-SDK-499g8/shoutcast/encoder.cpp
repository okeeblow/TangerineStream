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
#include "encoder.h"
#include "encode_lame.h"
#include "encode_ogg.h"

const char *Encoders::enumNames(int n) {
  if(n==0) return "Lame MP3 encoder v3.91";
#ifdef USEOGG
  if(n==1) return "Ogg Vorbis encoder RC3";
#endif
  return NULL;
}

Encoder *Encoders::createInstance(int n) {
  if(n==0) return new EncoderLame();
#ifdef USEOGG
  if(n==1) return new EncoderOgg();
#endif
  return NULL;
}