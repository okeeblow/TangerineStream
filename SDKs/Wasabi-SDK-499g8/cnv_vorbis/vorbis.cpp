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
#include "vorbis.h"
#include "../studio/wac.h"

VorbisDec::VorbisDec() 
{
	cur_link=0;
	theFile=0;
	pReader=0;
	needseek=-1;
}

VorbisDec::~VorbisDec() {
	vorbis_close();
}

static vorbis_comment* vorbis_get_meta(OggVorbis_File* file)
{
	return file->seekable ? file->vc+file->current_link : file->vc;
}

int VorbisDec::do_open(const char * _url)
{

	if (!url.isempty())
	{
		if (STRICMP(_url,url))
		{
			url=_url;
			vorbis_close();
		}
	}
	else url=_url;
	if (!theFile)
	{
		if (vorbis_open()) {pReader=0;return 0;}
	}
	cur_link=theFile->current_link;
	return 1;

}

bool is_bad_field(const char * x);

static void do_infos(MediaInfo * infos,OggVorbis_File * theFile,vorbis_comment * vc)
{
	static const char * stdfields[]={"title","artist","album","comment","genre","tracknumber","date"};
	static const int n_stdfields=(sizeof(stdfields)/sizeof(stdfields[0]));
	infos->setLength((int)(ov_time_total(theFile,-1)*1000.0));
	vorbis_info* i=ov_info(theFile,-1);
	
	UINT len=infos->getReader()->getLength();

	double t=ov_time_total(theFile,-1);
	
	int brate;
	
	if (t==OV_EINVAL || t<=0 || len==-1)
	{
		brate=i->bitrate_nominal;
	}
	else
	{
		brate=(UINT)(((double)len*8.0)/t);
	}

	if (i->channels<=2) infos->setInfo(StringPrintf("%ikbps %s %i.%ikhz", brate/1000, i->rate==1 ? "mono" : "stereo",i->rate/1000,(i->rate/100)%10));
	else infos->setInfo(StringPrintf("%ikbps %ichannels %i.%ikhz", brate/1000, i->rate,i->rate/1000,(i->rate/100)%10));

	int n;

	for(n=0;n<n_stdfields;n++)
	{
		infos->setData(stdfields[n],"", MediaInfo::INFO_EDIT_STRING|MediaInfo::INFO_VISIBLE);
	}

	for (n=0;n<vc->comments;n++)
	{
		char temp[256];
		char * tp=temp;
		char * ptr=vc->user_comments[n];
		while(*ptr && *ptr!='=')
		{
			*(tp++)=*(ptr++);
		}
		*tp=0;


		if (!*ptr)
		{
			infos->setData("comment",vc->user_comments[n],MediaInfo::INFO_EDIT_STRING|MediaInfo::INFO_VISIBLE);//broken shit
		}
		else if (!is_bad_field(temp))
		{
			infos->setData(temp,ptr+1,MediaInfo::INFO_EDIT_STRING|MediaInfo::INFO_VISIBLE);
		}
	}

}

int VorbisDec::getInfos(MediaInfo *infos) {
//	return 0;
	pReader=infos->getReader();
	if(!pReader) return 0;

	const char * filename=infos->getFilename();
	if (!do_open(filename)) return 0;

	vorbis_comment * vc=0;

	DeferredUpdate::sync.Enter();

	foreach(DeferredUpdate::list)
		DeferredUpdate * du=DeferredUpdate::list.getfor();
		if (!STRICMP(du->filename,filename)) {vc=&du->info;break;}
	endfor
		
	if (!vc) vc=vorbis_get_meta(theFile);

	do_infos(infos,theFile,vc);

	DeferredUpdate::sync.Leave();

	pReader=0;
	return 1;
}

int VorbisDec::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
	pReader=infos->getReader();
	if(!pReader) return 0;

	if (!do_open(infos->getFilename())) return 0;

	if(needseek!=-1) {
		vorbis_seek((double)needseek/1000.0);
		needseek=-1;
	}

	int n_link=-1;
	float ** channels;
	int nsam;
	while(1)
	{
		nsam=ov_read_float(theFile,&channels,576,&n_link);
		if ((signed)nsam<0)
		{
			if (nsam==OV_HOLE) continue;
			return 0;
		}
		break;
	}
	if (nsam<=0) return 0;
	if (n_link!=cur_link)
	{
		getInfos(infos);
		cur_link=n_link;
	}

	vorbis_info* vi=ov_info(theFile,-1);


	if (buffer.getSize()<nsam*vi->channels) buffer.setSize(nsam*vi->channels);

	float * bufptr=buffer.getMemory();

	{
		int n,p=0,c;
		if (vi->channels==6)
		{
			for(n=0;n<nsam;n++)
			{
				bufptr[p++]=channels[0][n];
				bufptr[p++]=channels[2][n];
				bufptr[p++]=channels[1][n];
				bufptr[p++]=channels[5][n];
				bufptr[p++]=channels[3][n];
				bufptr[p++]=channels[4][n];
			}
		}
		else for(n=0;n<nsam;n++)
		{
			for(c=0;c<vi->channels;c++)
			{	
				bufptr[p++]=channels[c][n];
			}
		}
	}


	ChunkInfosI *ci=new ChunkInfosI();

	ci->addInfo("srate",vi->rate);
	ci->addInfo("bps",32);
	ci->addInfo("nch",vi->channels);

	chunk_list->setChunk("FLOAT",bufptr,nsam*vi->channels*sizeof(float),ci);

	return 1;
}

int VorbisDec::corecb_onSeeked(int newpos) {
	needseek=newpos;
  return 0;
}

size_t VorbisDec::read_func(void *ptr, size_t size, size_t nmemb, VorbisDec *v)
{
	return v->pReader->read((char*)ptr,size*nmemb);
}


int VorbisDec::seek_func(VorbisDec *v, ogg_int64_t offset, int whence)
{
	svc_fileReader * r=v->pReader;
	if (!r->canSeek()) return -1;
	int l=r->getLength();
	if (l<=0) return -1;
	int o;
	if (whence==SEEK_SET) o=(int)offset;
	else if (whence==SEEK_END) o=l+(int)offset;
	else if (whence==SEEK_CUR) o=r->getPos()+(int)offset;
	else return -1;
	return v->pReader->seek(o);
}

int VorbisDec::close_func(VorbisDec *v)
{
	return 0;
}


__int64 VorbisDec::tell_func(VorbisDec *v)
{
	return (int)v->pReader->getPos();
}

/*
  size_t (*read_func)  (void *ptr, size_t size, size_t nmemb, void *datasource);
  int    (*seek_func)  (void *datasource, ogg_int64_t offset, int whence);
  int    (*close_func) (void *datasource);
  long   (*tell_func)  (void *datasource);
*/

void* VorbisDec::callbacks[4]=
{
	read_func,seek_func,close_func,tell_func
};

void vorbisfile_free(OggVorbis_File* f)
{
	if (f)
	{
		ov_clear(f);
		free(f);
	}
}

OggVorbis_File * VorbisDec::vorbisfile_create()
{
	OggVorbis_File* vf=new OggVorbis_File;
	if (!vf) {return 0;}
	memset(vf,0,sizeof(OggVorbis_File));
	pReader->seek(0);
	if (ov_open_callbacks(this,vf,0,0,*(ov_callbacks*)&callbacks))
	{
		vorbisfile_free(vf);
		return 0;
	}
	return vf;
}

void VorbisDec::vorbis_close()
{
	if (theFile)
	{
		vorbisfile_free(theFile);
		theFile=0;
		DeferredUpdate::Flush();
	}
}

int VorbisDec::vorbis_open()
{
	vorbis_info* i;

	theFile=vorbisfile_create();
	if (!theFile) return 1;

	i=ov_info(theFile,-1);
	if (!i)
	{
		vorbis_close();
		return 1;
	}
	
	vorbis_flush();
	return 0;
}

void VorbisDec::vorbis_flush()
{
}

int VorbisDec::canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype)
{
	if (chunktype || !name || !reader) return 0;
	if (reader->canSeek())
	{
		DWORD dw=0;
		reader->read((char*)&dw,4);
		reader->seek(0);
		if (dw!='SggO') return 0;

		{
			ogg_sync_state   sync;
			ogg_stream_state stream;
			ogg_page		 page;
			ogg_packet		 packet;

			int state=0;
			int read_total=0;

			memset(&sync,0,sizeof(sync));
			memset(&stream,0,sizeof(stream));
			memset(&page,0,sizeof(page));
			memset(&packet,0,sizeof(packet));
			ogg_sync_init(&sync);

			for(;;)
			{
				enum {PAGEBUF = 0x200, MAX=0x10000};//dont read more than 64k
				char * buf = ogg_sync_buffer(&sync,PAGEBUF);
				int read = reader->read(buf,PAGEBUF);
				if (read<=0) {state = -1;break;}
				read_total+=read;
				ogg_sync_wrote(&sync,read);
				
				int ret = ogg_sync_pageout(&sync,&page);
				if (ret>0) break;
				if (ret<0) {state=ret;break;}
				if (read_total>MAX) {state=-1;break;}
			}

			reader->seek(0);
			if (state==0)
			{
				ogg_stream_init(&stream,ogg_page_serialno(&page));
				ogg_stream_pagein(&stream,&page);
				
				if (ogg_stream_packetout(&stream,&packet)>0)
				{
					oggpack_buffer opb;

					oggpack_readinit(&opb,packet.packet,packet.bytes);

					{
						char buffer[6];
						int packtype=oggpack_read(&opb,8);
						
						{
							char * out = buffer;
							int bytes = sizeof(buffer);
							while(bytes--){
								*out++=(char)oggpack_read(&opb,8);
							}
						}

						if(!memcmp(buffer,"vorbis",sizeof(buffer))) state = 1;
					}
				}
				
			}

			ogg_sync_clear(&sync);
			ogg_stream_clear(&stream);
			
			return state>0;
		}
	}
	return !STRICMP(Std::extension(name),"ogg");
}


int VorbisDec::canSupportContentType(const char *contenttype)
{
	return !STRICMP(contenttype,"ogg") || !STRICMP(contenttype,"vorbis") || !STRICMP(contenttype,"application/x-vorbis") || !STRICMP(contenttype,"application/x-ogg") || !STRICMP(contenttype,"application/vorbis") || !STRICMP(contenttype,"application/ogg");
}


void VorbisComment::operator=(vorbis_comment & vc)
{
	Clear();
	Init();
	int n;
	for(n=0;n<vc.comments;n++) Add(vc.user_comments[n]);
}