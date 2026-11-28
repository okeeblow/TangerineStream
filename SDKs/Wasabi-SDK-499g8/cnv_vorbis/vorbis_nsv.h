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
#ifndef _VORBIS_NSV_H
#define _VORBIS_NSV_H

#include <studio/services/svc_mediaconverter.h>
#include <studio/services/servicei.h>
#include <core/corecb.h>
#include <studio/wac.h>
#include <bfc/attribs/cfgitemi.h>
#include <bfc/attribs/attrint.h>

#include <vorbis/vorbisfile.h>

class VorbisDecNsv : public svc_mediaConverterI {
public:
  VorbisDecNsv();
  virtual ~VorbisDecNsv();

  // service
  static const char *getServiceName() { return "Vorbis Decoder"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype);
  
  virtual const char *getConverterTo() { return "FLOAT"; }

  virtual int getInfos(MediaInfo *infos) {return 0;}

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

//	virtual int getLatency(void) { return 0; }

  // callbacks
//	virtual int corecb_onSeeked(int newpos);

//	virtual int canSupportContentType(const char *contenttype);
private:

	MemBlock<float> buffer;

	ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
	ogg_stream_state os; /* take physical pages, weld into a logical
			  stream of packets */
	ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       op; /* one raw packet of data for decode */
  
	vorbis_info      vi; /* struct that stores all the static vorbis bitstream
			  settings */
	vorbis_comment   vc; /* struct that stores all the bitstream user comments */
	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */
	int packets;

};
#endif
