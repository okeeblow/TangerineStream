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
#ifndef _PCMDSOUND_H
#define _PCMDSOUND_H

#include <windows.h>
#include <dsound.h>
#include "../studio/services/svc_mediaconverter.h"
#include "../studio/services/servicei.h"
#include "../studio/corecb.h"
#include "../studio/wac.h"

class PcmDSound : public svc_mediaConverterI {
public:
  PcmDSound();
  virtual ~PcmDSound();

  // service
  static const char *getServiceName() { return "PCM to DirectSound converter v2.0"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(chunktype && !STRICMP(chunktype,"PCM")) return 1;
    return 0;
  }
  virtual const char *getConverterTo() { return "DSOUND"; }

  virtual int getInfos(MediaInfo *infos) { return 0; }

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void);

  // core callbacks
  virtual int corecb_onStopped() { ShutDown(); return 0; }
  virtual int corecb_onVolumeChange(int newvol) { SetVolume(newvol); return 0; }
  virtual int corecb_onPaused() { Pause(TRUE); return 0; }
  virtual int corecb_onUnpaused() { Pause(FALSE); return 0; }
  virtual int corecb_onSeeked(int newpos) { do_flush(); return 0; }
  virtual int corecb_onAbortCurrentSong() { do_flush(); return 0; }

private:
  CRITICAL_SECTION cs;

  bool bInitted;
  HWND m_hwnd;
  int paused;
  int second_started;
  int buffer_size;
  int last_samplerate, last_nchannels, last_bits;
  LPDIRECTSOUND lpDS;
  LPDIRECTSOUNDBUFFER primary, second;
  int bytes_per_sample;
  HANDLE thread_id;
  volatile int time_to_die;
  volatile int write_pos;
  volatile unsigned _int64 play_pos;
  volatile unsigned _int64 buf_write_pos;
  int last_volume;
  int last_pan;
	int ds_initialized;

  int last_pause;
  int g_underrunning;
  int g_needstart;
  int playing;
  volatile unsigned int bytes_in_dbuf;
    
  int ResampleAndWrite(char *insamples, int bytes, int bps, int nch, int srate, bool *killswitch);

  static DWORD WINAPI PcmDSound::_thread(LPVOID lpData);
  int thread();
  
  int do_open(void);
  int do_close(void);
  int do_write(char *buf, int len, bool *killswitch);
  int do_getouttime(void);
  int do_getwritetime(void);
  void do_flush(void);

// buflib
  int _cbufferlen;//=2*24*1024*4;

  char *cbuffer;
  char *head, *tail;
  int bytes_in_buf;

  void bufReset(void) { head = tail = cbuffer; bytes_in_buf = 0; }
  int bufOpen(int blen) { 
    _cbufferlen=blen;
    if ((cbuffer = (char *)GlobalAlloc(GPTR, _cbufferlen+8)) == NULL) return 0;
    bufReset();
    return 1;
  }
  int bufPutBytes(char *p, int n);
  int bufGetBytes(char *p, int n);
  int bufAvailable(void) { return _cbufferlen - bytes_in_buf; }
  int bufInBuf(void) { return bytes_in_buf; }
  int bufClose(void);

  void ShutDown();
  void Pause(int pause);
  int SetVolume(int v);
  int SetPan(int pan);

  MediaInfo *m_infos;
};
#endif
