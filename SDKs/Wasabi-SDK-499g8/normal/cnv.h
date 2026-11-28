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
#ifndef _CNV_H
#define _CNV_H

#include "../studio/services/svc_mediaconverter.h"
#include "../bfc/msgpump.h"

class Analyzer : public svc_mediaConverterI {
public:
  static const char *getServiceName() { return "Normalization Analyzer"; }
  Analyzer();
  virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype);
  virtual const char *getConverterTo() { return "PCM"; }
  virtual int getInfos(MediaInfo *infos) { return 0; }
  virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);
  virtual int sortPlacement(const char *otherconverter) { return -1; }

  virtual int corecb_onNextFile();
  virtual int corecb_onSetNextFile(const char *playstring);

  // any one of these means our value is no good
  virtual int corecb_onSeeked(int newpos);
  virtual int corecb_onStopped();
  virtual int corecb_onAbortCurrentSong();
  virtual int corecb_onEQStatusChange(int newval);
  virtual int corecb_onEQPreampChange(int newval);

private:
  int active;
  int abort_current;
  String cur_playstring, next_playstring;
  MemBlock<double> lsamples, rsamples;
  unsigned int samples_processed;
};

struct Nmsg {
  int type;
  String string;
  float floaty;
  enum {
    SETVAL, SETTEXTFEED
  };
};

class Pumpo : public MessagePumpT<Nmsg> {
public:
  virtual void pumpCallback(Nmsg *msg);
};
extern Pumpo *pimpo;

#endif
