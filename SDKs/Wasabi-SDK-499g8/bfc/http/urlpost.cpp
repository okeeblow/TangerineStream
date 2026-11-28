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
// contributed by soundsys, aka Regis Nebor (rnebor@hotmail.com)

#include <jnetlib/jnetlib.h>
#include "httppost.h"
#include "urlpost.h"

#include <bfc/std.h>
#include <studio/api.h>

UrlPost::UrlPost(const char *_url,const char *content, int len, const char *filename, const char *useragent) {
  url = _url;
  dns = new JNL_AsyncDNS();
  httpPusher = new JNL_HTTPPost(dns);
  connected = 0;
  fp = NULL;
  size = 0;
  buffer.setSize(4096);

  if (useragent != NULL)
    httpPusher->addheader(StringPrintf("User-Agent:%s", useragent));
  else 
   httpPusher->addheader(StringPrintf("User-Agent:%s build %i (Mozilla)", WASABI_API_APP->main_getVersionString(), WASABI_API_APP->main_getBuildNumber()));
  httpPusher->addheader("Accept:*/*");

  m_contents=(char*)malloc(len+1);
  content_len=len+1;
  strncpy(m_contents,content,len);




  if(filename != NULL) {
    tmpfilename = filename;
    fp = fopen(tmpfilename, "wb");
    ASSERT(fp != NULL);
  }
  JNL::open_socketlib();
  ready = 0;
}

UrlPost::~UrlPost() {
  if (ready != 1 && !tmpfilename.isempty()) {
    unlink(tmpfilename.getValue());
  }
  shutdown();
}

int UrlPost::run() {
  ASSERT(httpPusher != NULL);
  if (ready != 0) return ready;
  if (!connected) {

httpPusher->setcontents(m_contents,content_len);


    httpPusher->connect(url, 1);
    connected = 1;
    return 0;
  }
  int r = httpPusher->run();
  if (r == -1) {
    shutdown();
    ready = -1;
    return -1;
  }
  
  int bytes_read = 0;
  while (httpPusher->bytes_available() > 0) {
    char tmpbuf[4096];
    int bytes = 0;

    bytes = httpPusher->get_bytes(tmpbuf, 4096);
    if(buffer.getSize() <= size + bytes_read + bytes)
       buffer.setSize(size + bytes_read + bytes);
    buffer.setMemory(tmpbuf, bytes, size+bytes_read);
    bytes_read += bytes;
    if(bytes_read > 0 && fp != NULL) {
    	fwrite(buffer, bytes_read, 1, fp);
    	buffer.zeroMemory();
      size += bytes_read;
      bytes_read = 0;
    }
  }
  size += bytes_read;
  
  if (r == 1) {
    shutdown();
    if(fp != NULL)
      fwrite(buffer, bytes_read, 1, fp);
    ready = 1;
  }
  return ready;
}

int UrlPost::shutdown() {
  delete httpPusher; httpPusher = NULL;
  delete dns; dns = NULL;
  if (fp != NULL) fclose(fp); fp = NULL;
  connected = 0;
  return 1;
}

const char *UrlPost::getErrorStr() {
  if (!httpPusher) return "";
  return httpPusher->geterrorstr();
}

void *UrlPost::getBuffer() {
	return buffer.getMemory();
}
