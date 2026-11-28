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
#include <windows.h>

/*
  void *fileOpen(const char *filename, const char *mode);
  void fileClose(void *fileHandle);
  int fileRead(void *buffer, int size, void *fileHandle);
  int fileWrite(const void *buffer, int size, void *fileHandle);
  int fileSeek(int offset, int origin, void *fileHandle);
  int fileTell(void *fileHandle);
  int fileGetFileSize(void *fileHandle);
  int fileExists(const char *filename);
  int fileRemove(const char *filename);
  // can't move directories between volumes on win32 :P
  int fileMove(const char *filename, const char *destfilename);
*/

#pragma warning(disable:4133)
#pragma warning(disable:4113)
#pragma warning(disable:4028)


#define PAGEBUF 0x4000

#ifdef _DEBUG
#define DEBUG(X) OutputDebugString(X)
#else
#define DEBUG(X)
#endif

int _inline _get_page(ogg_sync_state* oy,ogg_page * og,void* f,BOOL * eof/*,BOOL * eos*/,ULARGE_INTEGER * read_pos, ULARGE_INTEGER * size)
{
	char* buf;
	int v;
	int br;
	while(1)
	{
		v=ogg_sync_pageout(oy,og);

		if (v>0)
		{
			DEBUG("read ok\n");
/*			if (*eof && oy->fill==oy->returned)
			{
				*eos=1;
			}*/
			return 0;
		}

		if (/*v<0 || */*eof)
		{
			DEBUG("can't read\n");
			return 1;
		}

		DEBUG("need more data\n");


		{
			buf=ogg_sync_buffer(oy,PAGEBUF);
			br=api->fileRead(buf,PAGEBUF,f);
			if (br<PAGEBUF || api->fileTell(f)==api->fileGetFileSize(f)) {DEBUG("EOF\n");*eof=1;}
			if (br==0) return 1;
			

			ogg_sync_wrote(oy,br);
		}
	}
}

typedef struct
{
	char * data;
	UINT data_ptr,size;
} WRITEBUF;

#define DEF_BUF_SIZE (1024*1024)//default 1 meg buffer

static void wb_init(WRITEBUF * wb,UINT max_size)
{
	wb->size=DEF_BUF_SIZE > max_size ? max_size : DEF_BUF_SIZE;
	
	wb->data_ptr=0;
	while(wb->size)
	{
		wb->data=(char*)malloc(wb->size);
		if (wb->data) break;
		wb->size>>=1;//handle low-memory conditions
	}
}

static void wb_deinit(void* f,WRITEBUF * wb)
{
	if (wb->data)
	{
		if (wb->data_ptr)
		{
			api->fileWrite(wb->data,wb->data_ptr,f);
			wb->data_ptr=0;
		}
		free(wb->data);
		wb->data=0;
	}
}

static void wb_write(void* f,WRITEBUF * wb,void * ptr,UINT size)
{
	if (!wb->data)
	{
		api->fileWrite(ptr,size,f);
	}
	else
	{
		if (wb->data_ptr+size<=wb->size)
		{
			memcpy(wb->data+wb->data_ptr,ptr,size);
			wb->data_ptr+=size;
		}
		else
		{
			api->fileWrite(wb->data,wb->data_ptr,f);
			api->fileWrite(ptr,size,f);
			wb->data_ptr=0;
		}
	}
}

static void write_page(HANDLE f,ogg_page* og,WRITEBUF * wb)
{
	wb_write(f,wb,og->header,og->header_len);//avoid stupidass read-write trashing
	wb_write(f,wb,og->body,og->body_len);
}




static bool modify_file(const char* url,vorbis_comment * vc,int links)	//change meta-info using ogg stuff and some low-level packet data hacks
{	//also used for stream save fix
	void *src,*dst;
	BOOL scream=1;
	String tmp;

	tmp=url;
	tmp+=".tmp";
	
	if (!api->fileMove(url,tmp)) return 0;

	src=api->fileOpen(tmp,"rb");
	dst=api->fileOpen(url,"wb+");
	if (!src || !dst)
	{
		if (src) api->fileClose(src);
		if (dst) api->fileClose(dst);
		api->fileRemove(url);
		api->fileMove(tmp,url);
		return 0;
	}

	

	{		
		WRITEBUF wb;
		ULARGE_INTEGER src_size,src_pos;
		
		ogg_sync_state   oy_src;
		ogg_stream_state os_src;
		ogg_stream_state os_dst;
		ogg_page         og_src;
		ogg_page         og_dst;
		ogg_packet       op;


		int serial;
		BOOL got_streams=0;
		int links_done=0;

		UINT packets;
		BOOL eof=0;//,bos=1,eos=0;


		src_size.LowPart=GetFileSize(src,&src_size.HighPart);
		src_pos.LowPart=src_pos.HighPart=0;

		{
			FILETIME ct;
			GetFileTime(src,&ct,0,0);
			SetFileTime(dst,&ct,0,0);

		}
		
		{
			DWORD fs_lo=src_size.LowPart;
			if (src_size.HighPart) fs_lo=-1;
			wb_init(&wb,fs_lo);
		}

		got_streams=0;
		ogg_sync_init(&oy_src);

		while(!_get_page(&oy_src,&og_src,src,&eof/*,&eos*/,&src_pos,&src_size))
		{
			BOOL need_reset=0;
			if (!got_streams)
			{
				serial=ogg_page_serialno(&og_src);

				ogg_stream_init(&os_src,serial);
				ogg_stream_init(&os_dst,serial);
				got_streams=1;
				need_reset=1;
			}
			else if (serial!=ogg_page_serialno(&og_src))
			{
				if (got_streams)
				{
					while(ogg_stream_flush(&os_dst,&og_dst))
					{
						write_page(dst,&og_dst,&wb);
					}

					ogg_stream_clear(&os_src);
					ogg_stream_clear(&os_dst);
					links_done++;
					if (vc)
					{
						if (links_done>=links) vc=0;
						else {vc++;DEBUG("advancing to next stream\n");}
					}

				}
				serial=ogg_page_serialno(&og_src);
				ogg_stream_init(&os_src,serial);
				ogg_stream_init(&os_dst,serial);

				need_reset=1;
			}

			if (need_reset)
			{
				packets=0;
				eof=0;//eos=0;bos=1;
			}

			ogg_stream_pagein(&os_src,&og_src);
			
			while(ogg_stream_packetout(&os_src,&op)>0)
			{
				if (vc && packets==1) vorbis_commentheader_out(vc,&op);
				ogg_stream_packetin(&os_dst,&op);
				if (vc && packets==1) ogg_packet_clear(&op);
				packets++;
			}

			while(packets==3 ? ogg_stream_flush(&os_dst,&og_dst) : ogg_stream_pageout(&os_dst,&og_dst))
			{
/*				if (bos)
				{
					bos=0;
					og_dst.header[5]|=0x02;		
				}*/

				write_page(dst,&og_dst,&wb);
			}
		}
		if (got_streams)
		{
			while(ogg_stream_flush(&os_dst,&og_dst))
			{
				write_page(dst,&og_dst,&wb);
			}

			ogg_stream_clear(&os_src);
			ogg_stream_clear(&os_dst);
			links_done++;
		}
		ogg_sync_clear(&oy_src);



		wb_deinit(dst,&wb);
		
		scream=!links_done;

	}
	api->fileClose(src);
	api->fileClose(dst);
	api->fileRemove(tmp);
	return 1;
}

/*
extern void     vorbis_comment_init(vorbis_comment *vc);
extern void     vorbis_comment_add(vorbis_comment *vc, char *comment); 
extern void     vorbis_comment_add_tag(vorbis_comment *vc, 
				       char *tag, char *contents);
extern char    *vorbis_comment_query(vorbis_comment *vc, char *tag, int count);
extern int      vorbis_comment_query_count(vorbis_comment *vc, char *tag);
extern void     vorbis_comment_clear(vorbis_comment *vc);
*/

bool is_bad_field(const char * x)
{
	return !STRICMP(x,"reader") || !STRICMP(x,"filename") || !STRICMP(x,"length") || !STRICMP(x,"info") || !STRICMP(x,"infoedit");
}

int VorbisDec::setInfos(MediaInfo *infos)
{
	svc_fileReader * r=infos->getReader();
	if (r) r->close();
	VorbisComment vc;

	const char * nam;
	int n=0;
	while(nam=infos->enumDataName(n))
	{
		if (!is_bad_field(nam))
		{
			const char * val=infos->enumDataValue(n);
			if (*val) vc.AddTag(nam,val);
//			int att=infos->enumDataAttributes(n);
//			if (!(att & MediaInfo::INFO_NOEDIT)) 
		}
		n++;
	}

	DeferredUpdate::Add(infos->getFilename(),vc);
	DeferredUpdate::Flush();

	return 1;
}

const char *VorbisDec::getInfosXmlGroup() { return "infoedit.vorbis";}

PtrList<DeferredUpdate> DeferredUpdate::list;
CriticalSection DeferredUpdate::sync;

void DeferredUpdate::Flush()
{
	sync.Enter();
	PtrList<DeferredUpdate> del_list;

	foreach(list)
		if (list.getfor()->Try()) del_list.addItem(list.getfor());
	endfor

	foreach(del_list)
		list.deleteItem(del_list.getfor());
	endfor

	sync.Leave();
}

void DeferredUpdate::Add(const char * fn,VorbisComment & vc)
{
	sync.Enter();
	DeferredUpdate * u=0;
	foreach(list)
		if (!STRICMP(list.getfor()->filename,fn))
		{
			u=list.getfor();
			break;
		}
	endfor
	if (!u)
	{
		u=new DeferredUpdate(fn,vc);
		if (u->Try()) delete u;
		else list.addItem(u);
	}
	else
	{
		u->info=vc;
		if (u->Try()) list.deleteItem(u);
	}
	sync.Leave();

}

bool DeferredUpdate::Try()
{
	return modify_file(filename,&info,1);
}