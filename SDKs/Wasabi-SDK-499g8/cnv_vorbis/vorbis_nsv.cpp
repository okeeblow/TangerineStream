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
#include "vorbis_nsv.h"
#include "../studio/wac.h"
#include <malloc.h>

VorbisDecNsv::VorbisDecNsv() {
	packets=0;
	
	memset(&oy,0,sizeof(oy));
	memset(&os,0,sizeof(os));
	memset(&og,0,sizeof(og));
	memset(&op,0,sizeof(op));
	memset(&vi,0,sizeof(vi));
	memset(&vc,0,sizeof(vc));
	memset(&vd,0,sizeof(vd));
	memset(&vb,0,sizeof(vb));

	ogg_sync_init(&oy);

}

VorbisDecNsv::~VorbisDecNsv() {

    ogg_stream_clear(&os);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);

	ogg_sync_clear(&oy);
}



int VorbisDecNsv::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
{

	Chunk * c=chunk_list->getChunk("OggV");
	if (!c) return 0;

	void * src_buf=c->getData();
	int siz=c->getSize();

	int buf_used=0;

	if (c>0)
	{
		void * b=ogg_sync_buffer(&oy,siz);
		memcpy(b,src_buf,siz);
		ogg_sync_wrote(&oy,siz);
		while(ogg_sync_pageout(&oy,&og)>0)
		{
			int serial=ogg_page_serialno(&og);
			if (!packets) ogg_stream_init(&os,serial);
			else if (serial!=os.serialno)
			{
				vorbis_block_clear(&vb);
				vorbis_dsp_clear(&vd);
				vorbis_comment_clear(&vc);
				vorbis_info_clear(&vi);

				ogg_stream_clear(&os);
				ogg_stream_init(&os,serial);
				packets=0;
			}
			if (!packets)
			{
				vorbis_info_init(&vi);
				vorbis_comment_init(&vc);
			}
			ogg_stream_pagein(&os,&og);
			while(ogg_stream_packetout(&os,&op)>0)
			{
				if (packets<3)
				{
					if(vorbis_synthesis_headerin(&vi,&vc,&op)<0) return 0;
				}
				else
				{
					float ** pcm;
					int samples;
					if(vorbis_synthesis(&vb,&op)==0) vorbis_synthesis_blockin(&vd,&vb);
					while((samples=vorbis_synthesis_pcmout(&vd,&pcm))>0)
					{
						int n,c;
						if (buffer.getSize()<samples * vi.channels) buffer.setSize(samples * vi.channels);
						float * bufmem=buffer.getMemory();

						for(n=0;n<samples;n++)
						{
							for(c=0;c<vi.channels;c++)
							{
								bufmem[buf_used++]=pcm[c][n];
							}							
						}
						vorbis_synthesis_read(&vd,samples);
					}
				}
				packets++;
				if (packets==3)
				{
					vorbis_synthesis_init(&vd,&vi);
					vorbis_block_init(&vd,&vb);
				}
			}
		}
	}

	if (packets<3) return 0;
	ChunkInfosI *ci=new ChunkInfosI();

	ci->addInfo("srate",vi.rate);
	ci->addInfo("bps",32);
	ci->addInfo("nch",vi.channels);

	chunk_list->setChunk("FLOAT",buffer,buf_used*sizeof(float),ci);
	
	buf_used=0;	

	return 1;
}

int VorbisDecNsv::canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype)
{
	if (chunktype && !stricmp(chunktype,"ogg_vorbis")) return 1;
    return 0;
}

