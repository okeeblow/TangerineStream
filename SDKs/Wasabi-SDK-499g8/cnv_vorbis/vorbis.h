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
#ifndef _VORBIS_H
#define _VORBIS_H

#include <studio/services/svc_mediaconverter.h>
#include <studio/wac.h>
#include <bfc/attribs/cfgitemi.h>
#include <bfc/attribs/attrint.h>

#include <vorbis/vorbisfile.h>

class VorbisComment : public vorbis_comment
{
	void _init() {memset((vorbis_comment*)this,0,sizeof(vorbis_comment));Init();}
public:
	VorbisComment(vorbis_comment &vc) {_init();*this=vc;}
	VorbisComment(VorbisComment &vc) {_init();*this=vc;}
	VorbisComment() {_init();}
	~VorbisComment() {Clear();}
	void operator=(vorbis_comment & vc);
	void operator=(VorbisComment & vc) {*this=(vorbis_comment)vc;}
	
	inline void Init() {vorbis_comment_init(this);}
	inline void Add(const char * comment) {vorbis_comment_add(this,(char*)comment);}
	inline void AddTag(const char * tag,const char * contents) {vorbis_comment_add_tag(this,(char*)tag,(char*)contents);}
	inline char * Query(const char * tag,int count=0) {return vorbis_comment_query(this,(char*)tag,count);}
	inline int Query(const char * tag) {return vorbis_comment_query_count(this,(char*)tag);}
	inline void Clear() {vorbis_comment_clear(this);}
};

class CriticalSection : public CRITICAL_SECTION
{
public:
	inline void Enter() {EnterCriticalSection(this);}
	inline void Leave() {LeaveCriticalSection(this);}
	CriticalSection() {InitializeCriticalSection(this);}
	~CriticalSection() {DeleteCriticalSection(this);}
	//BOOL TryEnter() {return TryEnterCriticalSection(this);}
};


class DeferredUpdate
{
public:
	String filename;
	VorbisComment info;
	bool Try();
	DeferredUpdate(const char * fn,vorbis_comment & vc) : filename(fn), info(vc) {}
	static PtrList<DeferredUpdate> list;
	static CriticalSection sync;
	static void Flush();
	static void Add(const char * fn,VorbisComment & vc);
};





class VorbisDec : public svc_mediaConverterI 
{
public:
  VorbisDec();
  virtual ~VorbisDec();

  // service
  static const char *getServiceName() { return "Vorbis Decoder"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype);
  
  virtual const char *getConverterTo() { return "FLOAT"; }

  virtual int getInfos(MediaInfo *infos);

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void) { return 0; }

  // callbacks
  virtual int corecb_onSeeked(int newpos);

	virtual int canSupportContentType(const char *contenttype);
	virtual int setInfos(MediaInfo *infos);
	virtual const char *getInfosXmlGroup();
private:

	MemBlock<float> buffer;

	svc_fileReader * pReader;

	OggVorbis_File * theFile;

	String url;
	int needseek,cur_link;


	void vorbis_close();
	void vorbis_flush();
	int vorbis_open();
	OggVorbis_File * vorbisfile_create();

	double vorbis_length() {return ov_time_total(theFile,-1);}
	double vorbis_getpos() {return ov_time_tell(theFile);}
	void vorbis_seek(double p) {ov_time_seek(theFile,(double)p);vorbis_flush();}
	UINT vorbis_read();
	int do_open(const char * url);

	static size_t VorbisDec::read_func(void *ptr, size_t size, size_t nmemb, VorbisDec *v);
	static int seek_func(VorbisDec *v, ogg_int64_t offset, int whence);
	static int close_func(VorbisDec *v);
	static __int64 tell_func(VorbisDec *v);
	static void * callbacks[4];
};

#endif
