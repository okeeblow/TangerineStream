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
#ifndef _STD_MKNCC
#define _STD_MKNCC

// note: this is endian-incompatible with win32's MAKEFOURCC
// otoh, it shows up nicely in a debug register ;)

#define MK4CC(a, b, c, d) ( (((unsigned long)a)<<24)|(((unsigned long)b)<<16)|(((unsigned long)c)<<8)|((unsigned long)d) )
#define MK3CC(b, c, d) ( (((unsigned long)b)<<16)|(((unsigned long)c)<<8)|((unsigned long)d) )
#define MK2CC(c, d) ( (((unsigned long)c)<<8)|((unsigned long)d) )

#endif
