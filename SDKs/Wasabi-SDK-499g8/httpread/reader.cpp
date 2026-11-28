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

#include "reader.h"
#include "httpread.h"
#include "resource.h"

#include <studio/wac.h>
#include <bfc/util/pathparse.h>

#include <jnetlib/jnetlib.h>
#include <jnetlib/httpget.h>

#ifdef close
#undef close
#endif
#ifdef open
#undef open
#endif
#ifdef read
#undef read
#endif

extern _bool config_guess_prebuffer;
extern _string config_proxy;
extern _bool config_proxy80;
extern _int config_buffer_size, config_prebuffer_size, config_prebuffer_min;
extern _bool config_allowseek;
extern _bool config_fullseek;
extern _int config_seekprebuffer;
extern _bool config_suppressstatus;

extern WAComponentClient *the;

class HttpReader {
public:
  HttpReader(const char *url, int start_offset=0, int total_len=0, int is_seek=0);
  ~HttpReader();
  
  int connect();
  int read(char *buffer, int length);
  void abort() { killswitch=1; }
  int bytesAvailable();
  int getContentLength() { 
    if(m_contentlength) return m_contentlength; 
    return -1; 
  }
  int canSeek() { 
    return (m_contentlength && 
      /* JF> this is correct but not as compatible: m_accept_ranges && */ 
      !m_meta_interval); 
  }
  
  int getPos() { return m_contentpos; }
  
  const char *getHeader(const char *header) { return httpGetter->getheader((char *)header); }

  void setMetaCB(MetaDataReaderCallback *cb) { metacb=cb; }

  static BOOL CALLBACK httpDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

private:
  JNL_HTTPGet *httpGetter;
  JNL_AsyncDNS *dns;

  char *m_AllHeaders;

  int buffer_size;
  int prebuffer_size,prebuffer_min;
  int need_prebuffer;
  int m_contentlength,m_contentpos;
  int m_accept_ranges;
  
  int proxy_enabled;
  String proxy;

  int killswitch;
  
  int m_meta_init,m_meta_interval,m_meta_pos,m_meta_size,m_meta_buf_pos;
  char m_meta_buf[4096];

  MetaDataReaderCallback *metacb;
  
  int guessed_prebuffer_size;
  
  char lpinfo[256];
  char force_lpinfo[256];
  String dlg_realm;
  String m_url;
};

HttpRead::HttpRead() : reader(NULL), hasConnected(0) {
}

HttpRead::~HttpRead() {
  close();
}

int HttpRead::isMine(const char *filename, int mode) {
  if (!STRNINCMP(filename,"http://") ||
      !STRNINCMP(filename,"icy://") ||
      !STRNINCMP(filename,"sc://") ||
      !STRNINCMP(filename,"uvox://")) return 1;
  return 0;
}

int HttpRead::open(const char *filename, int mode) {
  if (!isMine(filename, mode)) return 0;

  m_filename=filename;
  reader=new HttpReader(filename);
  return 1;
}

int HttpRead::bytesAvailable(int requested)
{
  int v=reader?reader->bytesAvailable():0;
  if (v > requested) return requested;
  return v;
}

int HttpRead::read(char *buffer, int length) {
  if (!hasConnected) {
    int res=reader->connect();
    if (!res) { return 0; }
    hasConnected=1;
  }
  return reader->read(buffer,length);
}

void HttpRead::close() {
  delete reader;
  reader=NULL;
}

void HttpRead::abort() {
  if (reader) reader->abort();
}

int HttpRead::getLength() {
  return reader ? reader->getContentLength() : -1;
}

int HttpRead::getPos() {
  return reader ? reader->getPos() : 0;
}

int HttpRead::canSeek()
{
  return (config_allowseek && reader && reader->canSeek())?(config_fullseek?1:-1):0; 
}

int HttpRead::seek(int position)
{ 
  if (reader&&reader->canSeek()&&config_allowseek) 
  {
    if (position == getPos()) return 0;
    hasConnected=0;
    int cl=reader->getContentLength();
    delete((HttpReader *)reader);
    reader=new HttpReader(m_filename,position,cl,1);
    return 0;
  }
  return -1; 
}

int HttpRead::hasHeaders() {
  return 1;
}

const char *HttpRead::getHeader(const char *header) {
  return reader ? reader->getHeader(header) : NULL;
}

void HttpRead::setMetaDataCallback(MetaDataReaderCallback *cb) {
  if (reader) reader->setMetaCB(cb);
}

//***********************

HttpReader::HttpReader(const char *url, int start_offset, int total_len, int is_seek)
{

//s_debugFile << "HTTPReader created" << endl;


  m_accept_ranges=0;
  buffer_size=(config_buffer_size*1024);
  prebuffer_size=(config_prebuffer_size*1024);
  prebuffer_min=(config_prebuffer_min*1024);
  guessed_prebuffer_size=!config_guess_prebuffer;
  if (is_seek)
  {
    prebuffer_min=prebuffer_size=config_seekprebuffer;
    guessed_prebuffer_size=1;
  }
  proxy_enabled=0;
  killswitch=0;
  need_prebuffer=0;
  m_contentlength=total_len;
  m_contentpos=start_offset;
  m_meta_init=m_meta_interval=m_meta_pos=m_meta_size=m_meta_buf_pos=0;
  m_meta_buf[0]=0;
  metacb=NULL;
  force_lpinfo[0]=0;
  lpinfo[0]=0;

  m_url=url;
  int use_proxy=1;
  if(config_proxy80 && STRSTR(url,":") && (!STRSTR(url,":80/") && STRSTR(url,":80")!=(url+STRLEN(url)-3)))
    use_proxy=0;
  
  JNL::open_socketlib();
  dns=new JNL_AsyncDNS();
  httpGetter=new JNL_HTTPGet(dns, buffer_size, use_proxy?(char *)config_proxy.getValue():NULL);
  httpGetter->addheader("Accept:*/*");
  if ( ! STRNICMP( url, "uvox://", 7 )) {
	httpGetter->addheader("User-Agent: ultravox/2.0");
  } else {
	httpGetter->addheader((const char *)StringPrintf("User-Agent:%s build %i",WASABI_API_APP->main_getVersionString(),WASABI_API_APP->main_getBuildNumber()).getValue());
  }

  if (start_offset > 0)
    httpGetter->addheader((const char*)StringPrintf("Range: bytes=%d-",start_offset));
  else
    httpGetter->addheader("Icy-Metadata:1");
  httpGetter->connect((char *)m_url.getValue(),start_offset>0);
  
  if (!config_suppressstatus) api->core_setCustomMsg(0, StringPrintf("[Connecting] %s",url));
}

HttpReader::~HttpReader()
{
  delete dns;
  delete httpGetter;
  JNL::close_socketlib();
}

BOOL CALLBACK HttpReader::httpDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam) {
  HttpReader *_this;
  switch (uMsg)
  {
    case WM_INITDIALOG: {
      SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)lParam);
      _this = (HttpReader *)GetWindowLong(hwndDlg,GWL_USERDATA);
      PathParser pp(_this->force_lpinfo[0] ? _this->force_lpinfo : _this->lpinfo, ":");
      if (pp.getNumStrings() >= 2) {
        SetDlgItemText(hwndDlg,IDC_LOGIN, pp.enumString(0));
        SetDlgItemText(hwndDlg,IDC_PASS, pp.enumString(1));
      }
      SetDlgItemText(hwndDlg,IDC_REALM,_this->dlg_realm);
    }
    return 1;
    case WM_COMMAND:
      _this = (HttpReader *)GetWindowLong(hwndDlg,GWL_USERDATA);
      if (LOWORD(wParam) == IDOK)
      {
        char name[120]="", pass[120]="";
        GetDlgItemText(hwndDlg,IDC_LOGIN,name,sizeof(name));
        name[sizeof(name)-1]='\0';
        GetDlgItemText(hwndDlg,IDC_PASS,pass,sizeof(pass));
        pass[sizeof(pass)-1]='\0';
        STRCPY(_this->force_lpinfo, StringPrintf("%s:%s", name, pass));
        EndDialog(hwndDlg,1);
      }
      else if (LOWORD(wParam) == IDCANCEL)
      {
        EndDialog(hwndDlg,0);
      }
    break;
  }
  return 0;
}

int HttpReader::connect() {
  while (!killswitch && httpGetter->run()==0 && httpGetter->get_status()==0) 
  {
    Sleep(50);
  }
  if (killswitch) return 0;
  if (httpGetter->get_status()==-1)
  {
    int code=httpGetter->getreplycode();
    if(code==401) 
    {
      // authorization required
      JNL_Connection *m_connection=httpGetter->get_con();
      char str[4096];
      while (m_connection->recv_lines_available() > 0) {
        char *wwwa="WWW-Authenticate:";
        m_connection->recv_line(str,4096);
        if (!str[0]) break; 
        if (!strnicmp(str,wwwa,strlen(wwwa))) {
          char *s2="Basic realm=\"";
          char *p=str+strlen(wwwa); while (*p== ' ') p++;
          if (!strnicmp(p,s2,strlen(s2))) {
            p+=strlen(s2);
            if (strstr(p,"\"")) {
              strstr(p,"\"")[0]=0;
              if (*p) {
                if(force_lpinfo[0]) {
                  force_lpinfo[0]=0; // invalid l/p
                } else WASABI_API_CONFIG->getStringPrivate(StringPrintf("HTTP-AUTH/%s",p),force_lpinfo,sizeof(force_lpinfo),"");
                if (!force_lpinfo[0] || lpinfo[0]) {
                  dlg_realm = p;
                  api->pushModalWnd();
                  RootWnd *pr=api->main_getRootWnd();
                  while(pr->getParent()) pr=pr->getParent();
                  if (!DialogBoxParam(the->gethInstance(),MAKEINTRESOURCE(IDD_HTTPAUTH),pr->gethWnd(),httpDlgProc,(long)this)) {
                    force_lpinfo[0]=0;
                  } else {
                    WASABI_API_CONFIG->setStringPrivate(StringPrintf("HTTP-AUTH/%s",p),force_lpinfo);
                  }
                  api->popModalWnd();
                }
                if (force_lpinfo[0]) {
                  const char *p=STRSTR(m_url,"http://");
                  if(p) {
                    p+=7;
                    StringPrintf tmp("http://%s@%s",force_lpinfo,p);
                    httpGetter->connect((char *)tmp.getValue());
                    return connect(); // recursive city
                  }
                }    
              }
            }
          }
          break;
        }
      }
    }
    api->core_setCustomMsg(0, StringPrintf("HTTP: can't connect (%i)",code));
    return 0;
  }
  if (httpGetter->getreplycode()<200 || httpGetter->getreplycode()>299 )
  {
    api->core_setCustomMsg(0, StringPrintf("HTTP: returned %i",httpGetter->getreplycode()));
    return 0;
  }

  need_prebuffer=1;
  return 1;
}

int HttpReader::bytesAvailable()
{
  int code=httpGetter->run();
  int ba=httpGetter->bytes_available();
  if (!ba && code) return -1;
  return ba;
}

int HttpReader::read(char *buffer, int length)
{
  if (httpGetter->get_con()->get_state()==JNL_Connection::STATE_CONNECTED && httpGetter->bytes_available()<prebuffer_min) {
    need_prebuffer=1;
  }

  if (need_prebuffer) {
    need_prebuffer=0;
  if (!config_suppressstatus) api->core_setCustomMsg(0, "Prebuffering ...");

    if(!guessed_prebuffer_size) {
      // wait for headers
      int s;
      do {
        s=httpGetter->run();
      } while(s==0 && httpGetter->get_status()!=2);
      // calculate the needed prebuffer size if it's a shoutcast stream
      char *icybr;
      if(icybr=httpGetter->getheader("icy-br")) {
        prebuffer_size=(ATOI(icybr)/8)*4096;
        prebuffer_min=(ATOI(icybr)/8)*1024;
        if(prebuffer_size>buffer_size) prebuffer_size=buffer_size;
      }
      guessed_prebuffer_size=1;
    }

    int last_pre=-1;
    while (httpGetter->bytes_available()<prebuffer_size && !killswitch) {
      int s=httpGetter->run();
//      JNL_Connection::state s = getter->get_state();
  //    if (s == JNL_Connection::STATE_ERROR || s == JNL_Connection::STATE_CLOSED) break;
      if (s==-1 || s==1) break;
      Sleep(50);
      if (last_pre!=httpGetter->bytes_available() && !killswitch) 
      if (!config_suppressstatus) api->core_setCustomMsg(0, StringPrintf(0, "Prebuffering : %i/%i bytes",httpGetter->bytes_available(),prebuffer_size));
    }
    if (!killswitch) {
      if (!config_suppressstatus) api->core_setCustomMsg(0, "Prebuffering done.");
    }
  }

  if(killswitch) return 0;
 
  // metadata filtering
  if(!m_meta_init) {
    const char *v;
    if(v=httpGetter->getheader("icy-metaint:")) {
      m_meta_interval=ATOI(v); 
    }
    if (!m_contentlength) 
    {
      if(v=httpGetter->getheader("content-length:")) {
        m_contentlength=ATOI(v);
      }
    }
    v=httpGetter->getheader("accept-ranges:");
    if (v&&STRCASESTR(v,"bytes")) m_accept_ranges=1;
    m_meta_init=1;
  }


  int error=0,recvBytes=0;
  while(length && !error && !killswitch) {
    int code=httpGetter->run();

    if(code!=0) error=1;

    // old metadata parsing
    /*if (httpGetter->bytes_available()>0) {
      int l=httpGetter->get_bytes(buffer,length);

      // metadata stuff
			if (m_meta_interval) {
				int x=l;
				unsigned char *buf=(unsigned char *)buffer;
				if (m_meta_size)// already in meta block
				{				
					int len=MIN(x,m_meta_size-m_meta_buf_pos);

					MEMCPY(m_meta_buf+m_meta_buf_pos,buf,len);
					m_meta_buf_pos+=len;

					if (m_meta_buf_pos==m_meta_size)
					{
						if(metacb) metacb->metaDataReader_onData(m_meta_buf,m_meta_size);
						m_meta_buf_pos=0;
						m_meta_size=0;
						m_meta_pos=0;
					}

					x-=len;
					if (x) MEMCPY(buf,buf+len,x);
				}
				else if (m_meta_pos+x > m_meta_interval) // block contains meta data somewhere in it, and we're not alreayd reading a block
				{
					int start_offs=m_meta_interval-m_meta_pos;
					int len;
					m_meta_size=((unsigned char *)buf)[start_offs]*16;

					len=MIN(x-start_offs-1,m_meta_size);

					if (len) MEMCPY(m_meta_buf,buf+start_offs+1,len);
					m_meta_buf_pos=len;

					if (m_meta_buf_pos==m_meta_size) // full read of metadata successful
					{
						x-=m_meta_size+1;
						if (x > start_offs) MEMCPY(buf+start_offs,buf+start_offs+1+m_meta_size,x-start_offs);
						if(metacb) metacb->metaDataReader_onData(m_meta_buf,m_meta_size);
						m_meta_buf_pos=0;
						m_meta_pos=-start_offs;
						m_meta_size=0;
					}
					else 
					{
						x=start_offs; // otherwise, there's only the first block of data
					}
				}
				if (x > 0) 
				{
					m_meta_pos+=x;
				}
				l=x;
			}

      length-=l;
      buffer+=l;
      recvBytes+=l;
    } else Sleep(50);*/

    while(1) {
      int len=httpGetter->bytes_available();
      if (m_meta_interval && m_meta_pos >= m_meta_interval) {
        unsigned char b;
        if (len>0 && httpGetter->peek_bytes((char*)&b,1) && len>(b<<4)) {
          char metabuf[4096];
          httpGetter->get_bytes(metabuf,1);
          httpGetter->get_bytes(metabuf,b<<4);
          if(metacb) metacb->metaDataReader_onData(metabuf,b<<4);
          //stream_metabytes_read+=(b<<4)+1;
          m_meta_pos=0;
        } else break;
      } else {
        len=MIN(length,len);
        if (m_meta_interval) len=MIN(m_meta_interval-m_meta_pos,len);
        if (len > 0) {
          len=httpGetter->get_bytes((char*)buffer,len);
          m_meta_pos+=len;
          //stream_bytes_read+=len;
          length-=len;
          buffer+=len;
          recvBytes+=len;
        } else Sleep(50);
        break;
      }
    }

/*    int s=httpGetter->get_con()->get_state();
    if(code==0) {*/
/*      char tmp[512];
      wsprintf(tmp,"[Connected] Retrieving list (%i bytes)", recvBytes);
      api->status_setText(tmp);*/
//    } else error=1;
  }
  m_contentpos+=recvBytes;
  return recvBytes;
}
