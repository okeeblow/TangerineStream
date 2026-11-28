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
#ifndef _HTTPPOST_H_
#define _HTTPPOST_H_

// contributed by soundsys, aka Regis Nebor (rnebor@hotmail.com)

#include <jnetlib/connection.h>

class JNL_HTTPPost
{
  public:
    JNL_HTTPPost(JNL_AsyncDNS *dns=JNL_CONNECTION_AUTODNS, int recvbufsize=16384, char *proxy=NULL);
    ~JNL_HTTPPost();

    void addheader(const char *header);
    void setcontents(const char *contents, int len);
    void connect(const char *url, int ver=0);

    int run(); // returns: 0 if all is OK. -1 if error (call geterrorstr()). 1 if connection closed.

    int   get_status(); // returns 0 if connecting, 1 if reading headers, 
                        // 2 if reading content, -1 if error.

    char *getallheaders(); // double null terminated, null delimited list
    char *getheader(char *headername);
    char *getreply() { return m_reply; }
    int   getreplycode(); // returns 0 if none yet, otherwise returns http reply code.

    char *geterrorstr() { return m_errstr;}

    int bytes_available();
    int get_bytes(char *buf, int len);
    int peek_bytes(char *buf, int len);

    int content_length() { char *p=getheader("content-length"); if (p) return atoi(p); return 0; }

    JNL_Connection *get_con() { return m_con; }

  protected:
    void reinit();
    void deinit();
    void seterrstr(char *str) { if (m_errstr) free(m_errstr); m_errstr=(char*)malloc(strlen(str)+1); strcpy(m_errstr,str); }

    void do_parse_url(char *url, char **host, int *port, char **req, char **lp);
    void do_encode_mimestr(char *in, char *out);

    JNL_AsyncDNS *m_dns;
    JNL_Connection *m_con;
    int m_recvbufsize;

    int m_http_state;

    int m_http_port;
    char *m_http_url;
    char *m_http_host;
    char *m_http_lpinfo;
    char *m_http_request;

    char *m_http_proxylpinfo;
    char *m_http_proxyhost;
    int   m_http_proxyport;


    char *m_contents;
    int content_len;
    char *m_sendheaders;
    char *m_recvheaders;
    int m_recvheaders_size;
    char *m_reply;

    char *m_errstr;
};

#endif // _HTTPPOST_H_
