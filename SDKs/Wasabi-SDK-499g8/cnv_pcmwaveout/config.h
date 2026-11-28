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
#ifndef _CONFIG_H
#define _CONFIG_H

#include "../attribs/cfgitemi.h"
#include "../attribs/attrint.h"

class Config : public CfgItemI {
public:
  Config() : CfgItemI("WaveOut output") { 
    priority=2;
    volflags=1;
    num_blocks=8;
    bufsize_ms=2000;
    blocksize_min=8192;
    blocksize_max=32768;
    out_dev=WAVE_MAPPER;
  }

  void registerAttributes() {
    bufsize_ms.setName("Buffer size (ms)");
    registerAttribute(&bufsize_ms);
    num_blocks.setName("Nb of blocks");
    registerAttribute(&num_blocks);
    blocksize_min.setName("Block size (min)");
    registerAttribute(&blocksize_min);
    blocksize_max.setName("Block size (max)");
    registerAttribute(&blocksize_max);
  }
	
	_int priority;
	_int volflags;
	_int num_blocks;

	_int  bufsize_ms;
	_int  blocksize_max, blocksize_min;
	_int  out_dev;
};

extern Config config;

#endif