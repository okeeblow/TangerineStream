/*

  Nullsoft WASABI Source File License

  Copyright 1999-2001 Nullsoft, Inc.

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


  Brennan Underwood
  brennan@nullsoft.com

*/

#ifndef _COREHANDLE_H
#define _COREHANDLE_H

#include "../studio/corecb.h"
#include "common.h"

// a helper class to access the playback cores within an object for you

typedef unsigned int CoreToken;

// Fwd References
class CfgItem;
class ItemSequencer;

class COMEXP CoreHandle {
public:
  enum { maincore_token=0 };

  CoreHandle(CoreToken token=maincore_token);
  CoreHandle(const char *name);
  virtual ~CoreHandle();

  const char *getSupportedExtensions(); //just the *.mp3 or whatever
  const char *getExtSupportedExtensions(); // including names
  const char *getExtensionFamily(const char *extension);

  void registerExtension(const char *extensions, const char *extension_name);
  void unregisterExtension(const char *extensions);

  int setNextFile(const char *playstring);

  int getStatus(); // returns -1 if paused, 0 if stopped and 1 if playing

  const char *getCurrent();
  int getNumTracks();
  int getCurPlaybackNumber();
  int getPosition();
  int getWritePosition();
  int setPosition(int ms);
  int getLength();

  // this method queries the core plugins directly, bypassing the db
  int getPluginData(const char *playstring, const char *name,
    char *data, int data_len, int data_type=0); // returns size of data

  unsigned int getVolume(); // 0..255
  void setVolume(unsigned int vol); // 0..255
  int getPan(); // -127..127
  void setPan(int bal); // -127..127

  // register here for general callbacks in core status.
  void addCallback(CoreCallback *cb);
  void delCallback(CoreCallback *cb);

  // get visualization data, returns 0 if you should blank out
  int getVisData(void *dataptr, int sizedataptr);
  int getLeftVuMeter();
  int getRightVuMeter();
  
  int registerSequencer(ItemSequencer *seq);
  int deregisterSequencer(ItemSequencer *seq);

  int getEqStatus(); // returns 1 if on, 0 if off
  void setEqStatus(int enable);
  int getEqPreamp(); // -127 to 127 (-20db to +20db)
  void setEqPreamp(int pre);
  int getEqBand(int band); // band=0-9
  void setEqBand(int band, int val);
  int getEqAuto(); // returns 1 if on, 0 if off
  void setEqAuto(int enable);

  void prev();
  void play();
  void pause();
  void stop();
  void next();

  void setPriority(int priority);
  int getPriority();

private:
  void userButton(int button);
  CoreToken token;
  int createdcore;
};

#endif
