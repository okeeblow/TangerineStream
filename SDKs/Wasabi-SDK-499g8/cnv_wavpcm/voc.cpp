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
#include "cnv_wavpcm.h"

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	DWORD rate;
	BYTE bps;
	BYTE channels;
	WORD fmt;
	BYTE unk[4];
} BLOCK9HDR;

#pragma pack(pop)



static DWORD calc_rate(BYTE input)
{
	return 1000000/(256-input);
}


class voc : public converter_base
{
private:
	
	enum
	{
		BLOCK_DATA,BLOCK_SILENCE
	};
	struct VocChunk
	{
		int nch,bps,srate;
		int offset,size;
		int type;
		int length_ms() {return MulDiv(size,1000,srate * nch * (bps/8));}
	};

	int inited;
	int cursor,current;
	PtrList<VocChunk> chunks;

	MemBlock<char> data;
	


	int init(MediaInfo * infos)
	{
		if (inited) return 1;

		svc_fileReader * reader = infos->getReader();
		if (!reader) return 0;

		int offset=0;
		int length = reader->getLength();
		reader->seek(0x14);
		reader->read((char*)&offset,2);
		
		int loopstart=0,loopcount=0;
		
		int extended_srate;
		int extended_pack;
		int extended_nch;
		bool have_extended=0;

		while(1)
		{
			int type,size;
			type=size=0;
			reader->seek(offset);
			if (reader->read((char*)&type,1)!=1) break;
			if (type==0) break;
			if (reader->read((char*)&size,3)!=3) break;
			offset+=4;
			if (offset+size>length) break;
			switch(type)
			{
			case 1:
				if (size>2)
				{
					BYTE header[2];
					reader->read((char*)&header,2);
					if (header[1]==0)//TODO: weird compressed types ?
					{
						VocChunk * temp = new VocChunk;
						temp->bps = 8;
						if (have_extended)
						{
							temp->nch = extended_nch;
							temp->srate = 1000000/(256-(int)header[0]) / extended_nch;
						}
						else
						{
							temp->nch = 1;
							temp->srate = 1000000/(256-(int)header[0]);
						}
						temp->offset = offset+2;
						temp->size = size-2;
						temp->type = BLOCK_DATA;
						chunks.addItem(temp);
					}
				}
				break;
			case 2:
				if (size>0 && chunks.getNumItems()>0)
				{
					VocChunk * temp = new VocChunk(*chunks.getLast());
					temp->offset = offset;
					temp->size = size;
					chunks.addItem(temp);
				}
				break;
			case 3:
				if (size==3)
				{
					int silence_length=0;
					int silence_rate=0;
					reader->read((char*)&silence_length,2);
					reader->read((char*)&silence_rate,1);
					VocChunk * temp = new VocChunk;
					temp->nch = 1;
					temp->bps = 8;
					temp->srate = 1000000/(256-silence_rate);
					temp->offset = -1;
					temp->size = (int)silence_length + 1;
					temp->type = BLOCK_SILENCE;
					chunks.addItem(temp);
				}
				break;
			case 9:
				if (size>12)
				{
					BLOCK9HDR header;
					reader->read((char*)&header,sizeof(header));
					int real_size = size - sizeof(header);
					real_size -= real_size % (header.bps / 8 * header.channels);
					if (real_size>0)
					{
						VocChunk * temp = new VocChunk;
						temp->nch = header.channels;
						temp->bps = header.bps;
						temp->srate = header.rate;
						temp->offset = offset+sizeof(header);
						temp->size = real_size;
						temp->type = BLOCK_DATA;
						chunks.addItem(temp);
					}
				}
				break;
			case 6:
				if (size==2)
				{//start loop
					loopcount = 0;
					reader->read((char*)&loopcount,2);
					loopstart = chunks.getNumItems();
				}
				break;
			case 7:
				if (size==0)
				{//end loop
					int loopend = chunks.getNumItems();
					while(loopcount)
					{
						for(int ptr=loopstart;ptr<loopend;ptr++) chunks.addItem(new VocChunk(*chunks.enumItem(ptr)));
						loopcount--;
					}
					loopstart=0;
				}
				break;
			case 8:
				if (size==4)
				{
					DWORD temp;
					reader->read((char*)&temp,4);
					extended_nch = (temp&0xFF000000) ? 2 : 1;
					extended_srate = (temp&0xFFFF);//useless


					extended_pack = (temp>>16)&0xFF;
					have_extended=1;
				}
				break;
			}
			offset+=size;
		}

		if (!chunks.getNumItems()) return 0;


		current = 0;
		cursor = 0;
		inited = 1;
		return 1;
	}
	
public:

	voc()
	{
		inited=0;
		chunks=0;
	}

	~voc()
	{
		chunks.deleteAll();
	}

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
	{
		if (!init(infos)) return 0;
		
		svc_fileReader * reader = infos->getReader();
		if (!reader) return 0;

		VocChunk * cur_chunk;

		if (need_seek >= 0)
		{
			for(current=0;current<chunks.getNumItems();current++)
			{
				cur_chunk = chunks.enumItem(current);
				int len_ms = cur_chunk->length_ms();
				if (len_ms>need_seek)
				{
					cursor = (cur_chunk->bps/8) * cur_chunk->nch * MulDiv(need_seek,cur_chunk->srate,1000);
					break;
				}
				need_seek-=len_ms;
			}
			need_seek=-1;
		}

		cur_chunk = chunks.enumItem(current);
		if (!cur_chunk) return 0;

		int delta = 576 * (cur_chunk->bps/8) * cur_chunk->nch;
		if (delta > cur_chunk->size - cursor) delta = cur_chunk->size - cursor;
		
		if (data.getSize()<delta) data.setSize(delta);
		char * ptr = data.getMemory();
		
		switch(cur_chunk->type)
		{
		case BLOCK_DATA:
			reader->seek(cur_chunk->offset + cursor);
			reader->read(ptr,delta);
			break;
		case BLOCK_SILENCE:
			memset(ptr,cur_chunk->bps==8 ? 0x80 : 0,delta);
			break;
		}

		ChunkInfosI *ci=new ChunkInfosI();
		ci->addInfo("srate",cur_chunk->srate);
		ci->addInfo("bps",cur_chunk->bps);
		ci->addInfo("nch",cur_chunk->nch);

		chunk_list->setChunk("PCM",ptr,delta,ci);
		
		cursor += delta;

		if (cursor>=cur_chunk->size)
		{
			cursor = 0;
			current++;
			if (current>=chunks.getNumItems()) return 0;
		}

		return 1;
	}

	virtual int getInfos(MediaInfo *infos)
	{
		if (!init(infos)) return 0;

		int len_ms=0;

		int info_srate=0,info_nch=0,info_bps=0;

		foreach(chunks)
			VocChunk * chunk = chunks.getfor();
			len_ms += chunk->length_ms();
			if (chunk->type==BLOCK_DATA)
			{
				if (!info_srate) info_srate = chunk->srate;
				if (!info_nch) info_nch = chunk->nch;
				if (!info_bps) info_bps = chunk->bps;
			}
		endfor

		if (info_bps && info_nch && info_srate) infos->setInfo(StringPrintInfo((info_bps/8)*info_nch*info_srate,info_nch,info_srate));
		infos->setLength(len_ms);
		infos->setTitle(Std::filename(infos->getFilename()));
		return 1;
	}

	virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype)
	{
		if (!name || chunktype || STRICMP(Std::extension(name),"VOC") || !reader || !reader->canSeek()) return 0;

		int len=reader->getLength();
		if (len<0x20) return 0;
		__try
		{
			char header[0x14];
			reader->read(header,0x14);
			WORD offset;
			reader->read((char*)&offset,2);
			return !memcmp(header,"Creative Voice File\x1a",0x14) && offset>=0x16 && offset<0x100 && offset<(unsigned)len;
		} __finally
		{
			reader->seek(0);
		}
		return 0;
	}

	static const char *getServiceName() { return "VOC converter"; }
};


converter_make_service(voc);