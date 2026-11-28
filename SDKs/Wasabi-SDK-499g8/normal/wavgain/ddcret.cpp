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
/*==========================================================================

	 ddcret.cpp  -  Don Cross, February 1993.

	 Returns a string for any given DDCRET value.

Revision history:

1993 February 16 [Don Cross]
	 Initial revision.  Contains 'const char *DDCRET_String(DDCRET)'.

==========================================================================*/

#include <stdio.h>
#include "ddc.h"

const char *DDCRET_String ( DDCRET retcode )
{
   switch ( retcode )
   {
	  case DDC_SUCCESS:          return "DDC_SUCCESS";
	  case DDC_FAILURE:          return "DDC_FAILURE";
	  case DDC_OUT_OF_MEMORY:    return "DDC_OUT_OF_MEMORY";
	  case DDC_FILE_ERROR:       return "DDC_FILE_ERROR";
	  case DDC_INVALID_CALL:     return "DDC_INVALID_CALL";
	  case DDC_USER_ABORT:       return "DDC_USER_ABORT";
	  case DDC_INVALID_FILE:     return "DDC_INVALID_FILE";
   }

   // If we get here, it is not a known retcode.

   static char buffer [80];

   sprintf ( buffer, "DDCRET(%d)", int(retcode) );

   return buffer;
}

/*--- end of file ddcret.cpp ---*/