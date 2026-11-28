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
#ifndef _BPL_H
#define _BPL_H

#include "plload.h"
#include "../bfc/xml/xmlwrite.h"
#include "../studio/xmlreader.h"

class B4sLoader : public PlaylistLoader, public XmlReaderCallbackI {
public:
  static const char *getServiceName() { return "b4s loader"; }

  virtual const char *getExtension() { return "b4s"; }
  virtual int readPlaylist(const char *filename);

  // xmlreader stuff
  virtual void xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params);
  virtual void xmlReaderOnEndElementCallback(const char *xmlpath, const char *xmltag);
  virtual void xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *s);
};

class BplLoader : public B4sLoader {
public:
  static const char *getServiceName() { return "bpl loader"; }
  virtual const char *getExtension() { return "bpl"; }
};

class XMLWrite;

class B4sWriter : public svc_playlistWriterI {
public:
  static const char *getServiceName() { return "b4s writer"; }

  B4sWriter();

  virtual const char *getExtension() { return "b4s"; }
  virtual const char *getDescription() { return "B4S playlist"; }

  virtual void enableMetadata(int enabled);

  virtual int beginWrite(const char *filename, int n, const char *label);
  virtual void writeEntry(const char *playstring);
  virtual void endWrite();

private:
  void xmlwrite(const char *playstring, const char *tag);
  XMLWrite *w;
  int enable_metadata;
  String path;
};

#endif
