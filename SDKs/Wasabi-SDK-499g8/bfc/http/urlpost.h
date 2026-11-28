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
#ifndef _URLPOST_H
#define _URLPOST_H

// contributed by soundsys, aka Regis Nebor (rnebor@hotmail.com)

#include <stdio.h>
#include <bfc/string.h>
#include <bfc/memblock.h>

class MemBlock;
class JNL_HTTPPost;
class JNL_AsyncDNS;

class UrlPost {
public:
  //UrlPost(const char *url, const char *filename, const char *useragent=NULL); GL> new constructor
  UrlPost(const char *url, const char *content, int len, const char *filename=NULL, const char *useragent=NULL);
  ~UrlPost();

public:
  int run();	// returns 1 when file is ready, 0 = wait, -1 = error
  int shutdown();

  int filesize() const { return size; }

  const char *getFilename() const { return tmpfilename; }

  const char *getErrorStr();

  void *getBuffer();

private:
  MemBlock<char> buffer;

  char *m_contents;
  int content_len;
  
  JNL_HTTPPost *httpPusher;
  JNL_AsyncDNS *dns;
  FILE *fp;
  String url, tmpfilename;
  int connected, ready;
  int size;
};

#endif
