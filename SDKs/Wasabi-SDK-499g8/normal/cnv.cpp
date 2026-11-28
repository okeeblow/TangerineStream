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

#include "cnv.h"

#include <common/metatags.h>
#include <core/corehandle.h>

#include "WavGain/gain_analysis.h"

static int inited, last_srate;

typedef signed short SSHORT;

Analyzer::Analyzer() {
  active = 0;
  abort_current = 0;
  samples_processed = 0;
}

int Analyzer::canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) {
  if (STREQLSAFE(chunktype, "PCM")) return 1;
  return 0;
}

int Analyzer::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  extern int learn_now;
  if (!learn_now) abort_current = 1;

  if (abort_current) return 1;

  if (chunk_list == NULL) return 1;

  Chunk *chunk = chunk_list->getChunk("PCM");

  if (chunk == NULL) return 1;

  int srate = chunk->getInfo("srate");
  if (srate == 0) return 1;
  if (!inited) {
    InitGainAnalysis(srate);
    inited = 1;
  } else {
    if (srate != last_srate) ResetSampleFrequency(srate);
  }
  last_srate = srate;

  int bits = chunk->getInfo("bps");
  if (bits != 16) return 1;
  int nch = chunk->getInfo("nch");
  if (nch < 1 || nch > 2) return 1;
  void *data = chunk->getData();
  if (data == NULL) return 1;
  SSHORT *samples = (SSHORT *)data;
  if (chunk->getSize() < 1) return 1;
  int numsamples = chunk->getSize() / (sizeof(SSHORT)*nch);

  if (lsamples.getSize() < numsamples) lsamples.setSize(numsamples);
  if (rsamples.getSize() < numsamples) rsamples.setSize(numsamples);
  for (int i = 0; i < numsamples; i++) {
    double d = (double)(samples[i*2]);
    lsamples[i] = d;
    if (nch > 1) {
      double d = (double)(samples[i*2+1]);
      rsamples[i] = d;
    }
  }
  AnalyzeSamples(lsamples, rsamples, numsamples, nch);

  samples_processed += numsamples;

  return 1;
}

int Analyzer::corecb_onNextFile() {
  // wrap up the previous song
  double rg = GetTitleGain();

OutputDebugString("end of song");
OutputDebugString(StringPrintf("ps %s rg %f abort_current %d", cur_playstring.getValue(), rg, abort_current));
  // store it?
  if (!cur_playstring.isempty() && rg >= -20000.0 && !abort_current) {
    Nmsg *nmsg = new Nmsg;
    nmsg->type = Nmsg::SETVAL;
    nmsg->string = cur_playstring;
    nmsg->floaty = (float)rg;
    pimpo->pushMessage(nmsg);
OutputDebugString("pushing data");
  }

  cur_playstring = next_playstring; next_playstring = NULL;
  abort_current = 0;
  samples_processed = 0;

  CoreHandle mycore(m_coretoken);
  corecb_onEQStatusChange(mycore.getEqStatus());
  corecb_onEQPreampChange(mycore.getEqPreamp());

  return 1;
}

int Analyzer::corecb_onSetNextFile(const char *playstring) {
  // check if we already have the data for this file

  // if not, here we go
  if (cur_playstring.isempty())
    cur_playstring = playstring;
  else
    next_playstring = playstring;

  return 1;
}

int Analyzer::corecb_onSeeked(int newpos) {
OutputDebugString("SOUGHT MOTHERFUCKER");
  abort_current = 1;
  return 1;
}

int Analyzer::corecb_onStopped() {
OutputDebugString("STOPPED MOTHERFUCKER");
  abort_current = 1;
  return 1;
}

int Analyzer::corecb_onAbortCurrentSong() {
OutputDebugString("ABORTIO MOTHERFUCKER");
  abort_current = 1;
  return 1;
}

int Analyzer::corecb_onEQStatusChange(int newval) {
  if (newval && CoreHandle(m_coretoken).getEqPreamp() != 0) {
OutputDebugString("STATUS MOTHERFUCKER");
    abort_current = 1;
  }
  return 1;
}

int Analyzer::corecb_onEQPreampChange(int newval) {
  if (newval && CoreHandle(m_coretoken).getEqStatus() != 0) {
OutputDebugString("PREAMP MOTHERFUCKER");
    abort_current = 1;
OutputDebugString("I abort thee preampo");
OutputDebugString(StringPrintf("newval is %d", newval));
  }
  return 1;
}

void sendStatus(const char *st);

void Pumpo::pumpCallback(Nmsg *msg) {
  switch (msg->type) {
    case Nmsg::SETVAL:
      api->metadb_setMetaData(msg->string, "Preamp", (char*)&msg->floaty, sizeof(float), MDT_FLOAT);
      DebugString("GRAAA %s <-- %f", msg->string.getValue(), msg->floaty);
      sendStatus(StringPrintf("Final preamp value: %4.2f", msg->floaty));
    break;
    case Nmsg::SETTEXTFEED:
      sendStatus(msg->string);
    break;
  }
  delete msg;
}

Pumpo *pimpo;
