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
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: vorbis encode-engine setup
 last mod: $Id: vorbisenc.h,v 1.8 2001/12/20 01:00:25 segher Exp $

 ********************************************************************/

#ifndef _OV_ENC_H_
#define _OV_ENC_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "codec.h"

extern int vorbis_encode_init(vorbis_info *vi,
			      long channels,
			      long rate,
			      
			      long max_bitrate,
			      long nominal_bitrate,
			      long min_bitrate);

extern int vorbis_encode_setup_managed(vorbis_info *vi,
				       long channels,
				       long rate,
				       
				       long max_bitrate,
				       long nominal_bitrate,
				       long min_bitrate);
  
extern int vorbis_encode_setup_vbr(vorbis_info *vi,
				  long channels,
				  long rate,
				  
				  float /* quality level from 0. (lo) to 1. (hi) */
				  );

extern int vorbis_encode_init_vbr(vorbis_info *vi,
				  long channels,
				  long rate,
				  
				  float base_quality /* quality level from 0. (lo) to 1. (hi) */
				  );

extern int vorbis_encode_setup_init(vorbis_info *vi);

extern int vorbis_encode_ctl(vorbis_info *vi,int number,void *arg);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


