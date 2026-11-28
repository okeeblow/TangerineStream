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
#ifndef _READER_H
#define _READER_H

#include <studio/services/svc_fileread.h>
#include <bfc/string.h>

class HttpReader;

class HttpRead : public svc_fileReaderI {
public:
  HttpRead();
  virtual ~HttpRead();
  static const char *getServiceName() { return "HTTP file reader"; }

  int isMine(const char *filename, int mode=SvcFileReader::READ);
  int open(const char *filename, int mode=SvcFileReader::READ);
  int read(char *buffer, int length);
  int write(const char *buffer, int length) { return 0; }
  int bytesAvailable(int requested);
  void close();
  void abort();

  int getLength();
  int getPos();
  int canSeek();
  int seek(int position);
  
  int hasHeaders();
  const char *getHeader(const char *header);

  void setMetaDataCallback(MetaDataReaderCallback *cb);

  int canPrefetch() { return 0; } // no info fetch on HTTP files
  
private:
  HttpReader *reader;
  int hasConnected;
  String m_filename;
};
#endif
