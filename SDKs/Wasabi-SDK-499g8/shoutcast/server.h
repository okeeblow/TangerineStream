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
#ifndef _SERVER_H
#define _SERVER_H

#include "../bfc/ptrlist.h"
#include "../bfc/string.h"
#include "../bfc/timerclient.h"
#include "shoutcast.h"

#include <winsock.h>
#include "../../jnetlib/jnetlib.h"

#include "encoder.h"

class Buffer // a thread-safe buffer
{
public:
  Buffer()
  {
    m_size=0;
    m_buffer=NULL;
    m_max=0;
    InitializeCriticalSection(&cs);
  }
  ~Buffer()
  {
    if(m_buffer) FREE(m_buffer);
    DeleteCriticalSection(&cs);
  }

  void add(void *data, int size)
  {
    if(size<=0) return;
    EnterCriticalSection(&cs);
    if(!m_buffer) 
    {
      m_max=size*4;
      m_buffer=(void *)MALLOC(m_max);
    }
    else if((size+m_size)>m_max)
    {
      while((size+m_size)>m_max) m_max*=2;
      m_buffer=(void *)REALLOC(m_buffer,m_max);
    }
    MEMCPY((char *)m_buffer+m_size, (char *)data, size);
    m_size+=size;
    LeaveCriticalSection(&cs);
  }
  int get(void *data, int size)
  {
    EnterCriticalSection(&cs);
    int s=MIN(size,m_size);
    if(s<=0) {
      LeaveCriticalSection(&cs);
      return 0;
    }
    MEMCPY(data, m_buffer, s);
    m_size-=s;
    MEMCPY((char *)m_buffer, (char *)m_buffer+s, m_size);
    LeaveCriticalSection(&cs);
    return s;
  }
  int getSize() { 
    EnterCriticalSection(&cs);
    int s=m_size;
    LeaveCriticalSection(&cs);
    return s;
  }
  void reset() {
    EnterCriticalSection(&cs);
    m_size=0;
    LeaveCriticalSection(&cs);
  }
  void *getBuffer() { return m_buffer; }
protected:
  void *m_buffer;
  int m_size;
  int m_max;
  CRITICAL_SECTION cs;
};

class Server : public TimerClientDI {
public:
  Server();
  ~Server();

  void init();

  virtual void timerclient_timerCallback(int id);

  void run_connection();
  void close_connection();

	int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);
  int getTimeout() { return (int)(GetTickCount()-state_timeout); }
  
  void initEncoderConfig(ContWnd *configwnd, int x, int y) { encoder->initConfig(configwnd,x,y); }
  void updateEncoderConfig() { encoder->updateConfig(); }
  void getEncoderConfig() { encoder->getConfig(); }
  void removeEncoderConfig() { encoder->removeConfig(); }
  
  void switchEncoder(int newenc);
  
  void loadConfig(const char *pre);
  void saveConfig(const char *pre);

  void setSongTitle(const char *s) { songtitle=s; send_title=1; }

  String name;
  String url;
  String genre;
  String aim;
  String icq;
  String irc;
  int pub;
  int title_upd;

  String host;
  int port;
  String passwd;
  int autoconnect;

  int encodernum;
  
  int nb_listeners;
  _int64 bytes_sent;
 
  enum {
    STATE_NONE,
    STATE_CONNECT,
    STATE_CONNECTWAIT,
    STATE_CONNECTERROR,
    STATE_LOGIN,
    STATE_LOGINWAIT,
    STATE_LOGINERROR,
    STATE_SENDHEADER,
    STATE_SEND,
    STATE_SENDERROR,
  };
  int state;
  DWORD state_timeout;

  DWORD last_listenerget;
  int listener_get_state;
  
  DWORD last_titleupd;
  int titleupd_state;

private:
  void sw(int s) { state=s; state_timeout=GetTickCount(); }
  void run_listener_get();
  void run_title_upd();
  
  void fixstring(const char *in, String &out);

  int initialized;
  
  Encoder *encoder;
  
  JNL_Connection *connection;
  JNL_HTTPGet *listenersget;
  JNL_HTTPGet *titleupdget;

  Buffer to_send;
  Buffer listener_buffer;

  String songtitle;
  int send_title;
};

extern PtrList<Server> g_servers;
extern PtrList<Encoder> g_encoders;

#endif
