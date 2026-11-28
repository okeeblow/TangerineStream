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
#ifndef _PCMWAVEOUT_H
#define _PCMWAVEOUT_H

#include <windows.h>
#include "../studio/services/svc_mediaconverter.h"
#include "../studio/services/servicei.h"
#include "../studio/corecb.h"

typedef __int64 int64;

#define MAX_NUM_BLOCKS 32

class PcmWaveout : public svc_mediaConverterI {
public:
  PcmWaveout();
  ~PcmWaveout();
  
  // service
  static const char *getServiceName() { return "PCM to Waveout converter"; }

  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) { 
    if(chunktype && !STRICMP(chunktype,"PCM")) return 1;
    return 0;
  }
  virtual const char *getConverterTo() { return "WAVEOUT"; }

  virtual int getInfos(MediaInfo *infos) { return 0; }

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

  virtual int getLatency(void) { if (bInitted) return do_getouttime()-do_getwritetime(); else return 0; }

  // core callbacks
  virtual int corecb_onStopped() { ShutDown(); return 0; }
  virtual int corecb_onVolumeChange(int newvol) { SetVolume(newvol); return 0; }
  virtual int corecb_onPaused() { Pause(TRUE); return 0; }
  virtual int corecb_onUnpaused() { Pause(FALSE); return 0; }
  virtual int corecb_onSeeked(int newpos) { Restart(); return 0; }
  virtual int corecb_onAbortCurrentSong() { Restart(); return 0; }

private:
  bool bInitted;

  private:

    int ResampleAndWrite(char *insamples, int bytes, int bps, int nch, int srate, bool *killswitch);
    void do_set_priority(void);
    void do_set_blocksizes(void);
    int do_open(void); 
    void do_close(void);
    int do_write(char *buf, int len, bool *killswitch);
    int do_canwrite(void);
    int do_isplaying(void);
    void do_flush(void);
    int do_getouttime(void);
    int do_getwritetime(void);
    
    static DWORD WINAPI _threadproc(LPVOID p);
    DWORD ThreadProc(void);

    void _setvol(void);
    void do_samples_altvol(char *in, int blen);

    void Pause(int pause);
    void ShutDown(void);
    void FlushSamples(bool *killswitch) { do_write(NULL,0,killswitch); }
    void Restart(void) { do_flush(); }
    int SetVolume(int volume);
    int SetPan(int pan);

    int              m_min_blocksize;
    int              m_max_blocksize;
    int              m_num_blocks;
    char            *m_buffer, *m_buffer_write, *m_buffer_read;
    int              m_buffer_length, m_buffer_valid,m_buffer_inlength;
    HWAVEOUT         m_hWaveOut;
    WAVEHDR          m_wave_headers[MAX_NUM_BLOCKS];
    int				       m_pause, m_writeall, m_wavecnt, m_prebuf;
    volatile int		 m_quit_flag;
    HANDLE           m_hSem,m_hEvent, m_hThread;
    CRITICAL_SECTION m_cs;
    int              m_bytes_per_sec, m_outtime_interp;
    int64            m_outtime,m_writetime;
    int a_v,a_p;
    unsigned char *vol_tab, use_vol_tab;
};
#endif
