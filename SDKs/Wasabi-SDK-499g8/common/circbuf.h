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
#ifndef _CIRCBUF_H
#define _CIRCBUF_H

// originally isolated from the crossfader plugin
class CircBuf {
public:
  CircBuf() {
    _cbufferlen=0;
    cbuffer=NULL;
    head=tail=0;
    bytes_in_buf=0;
  }

  ~CircBuf() {
    close();
  }

  int open(int blen) {
    close();
    _cbufferlen=blen;
    if ((cbuffer = (char *)MALLOC(_cbufferlen+8)) == NULL) return 0;
    reset();
    return 1;
  }

  int isOpen() { return cbuffer != NULL; }

  void reset() {
    head = tail = cbuffer;
    bytes_in_buf = 0;
  }
  int getSize() { return _cbufferlen; }

  int putBytes(char *p, int n) {
    if (n <= 0) return 1;
    int put = n;
    int l2;
    if (!cbuffer) return 0;
    l2 = (cbuffer+_cbufferlen)-head;
    if (l2 <= n) {
      memcpy(head,p,l2);
      head=cbuffer;
      p+=l2;
      n-=l2;
    }
    if (n) {
      memcpy(head,p,n);
      head+=n;
      p+=n;
    }
    bytes_in_buf += put;
    return put;
  }

  int getBytes(char *p, int n) {
    int got = 0;
    if (!cbuffer) return 0;
    if (bytes_in_buf <= 0) return 0;
    if (n > bytes_in_buf) n=bytes_in_buf;
    bytes_in_buf-=n;
    got=n;
    if (tail+n >= cbuffer+_cbufferlen) {
      int l1=cbuffer+_cbufferlen-tail;
      n-=l1;
      memcpy(p,tail,l1);
      tail=cbuffer;
      p+=l1;
      memcpy(p,tail,n);
      tail+=n;
    } else {
      memcpy(p,tail,n);
      tail+=n;
    }
    return got;
  }

  int available() {
    if (cbuffer) return _cbufferlen - bytes_in_buf;
    return 0;
  }

  int bytesInBuf() {
    if (cbuffer) return bytes_in_buf;
    return 0;
  }

  int close() {
    if (cbuffer) FREE(cbuffer);
    cbuffer = NULL;
    head = tail = cbuffer;
    bytes_in_buf = 0;
    _cbufferlen=0;
    return 1;
  }

private:
  int _cbufferlen;
  char *cbuffer;
  char *head, *tail;
  int bytes_in_buf;
};

#endif
