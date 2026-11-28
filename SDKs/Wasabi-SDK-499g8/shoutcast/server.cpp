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
#include "server.h"

PtrList<Server> g_servers;

extern _int retrydelay; // in seconds

Server::Server() {
  name="Unnamed server";
  genre="";
  url="http://www.shoutcast.com/";
  aim="N/A";
  icq="0";
  irc="#shoutcast";
  pub=1;
  title_upd=1;
  host="localhost";
  port=8000;
  passwd="changeme";
  autoconnect=0;
  encodernum=0;
  nb_listeners=0;
  bytes_sent=0;

  initialized=0;
  encoder=Encoders::createInstance(encodernum);
  connection=NULL;

  state=STATE_NONE;
  listener_get_state=0;
  last_listenerget=NULL;
  titleupd_state=0;
  last_titleupd=NULL;
  send_title=0;
}

Server::~Server() {
  delete(encoder);
  close_connection();
  timerclient_killTimer(100);
}

void Server::init() {
  timerclient_setTimer(100, 20);
}

void Server::timerclient_timerCallback(int id) {
  if (id == 100) {
    run_connection();
  }
}

int Server::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  if(!initialized) initialized=1; 

  if(!encoder) return 1;

  if(state!=STATE_SEND) return 1;

  if(!encoder->processData(infos,chunk_list,killswitch)) return 1;

  Chunk *c=chunk_list->getChunk(encoder->getConverterTo());
  if(c) {
    int s=c->getSize();
    if(s) to_send.add(c->getData(),s);

/*    FILE *fh=fopen("c:\\blah.ogg","ab");
    FWRITE(c->getData(),s,1,fh);
    FCLOSE(fh);*/
  }

  return 1;
}

void Server::run_connection() {
  switch(state) {
  case STATE_NONE: if(initialized&&autoconnect) sw(STATE_CONNECT); break;
  case STATE_CONNECT:
    connection=new JNL_Connection();
    connection->connect((char *)host.getValue(),port+1);
    sw(STATE_CONNECTWAIT);
    break;
  case STATE_CONNECTWAIT: {
    connection->run();
    int s=connection->get_state();
    if(s==JNL_Connection::STATE_CONNECTED) sw(STATE_LOGIN);
    if(s==JNL_Connection::STATE_ERROR || s==JNL_Connection::STATE_CLOSED) {
      close_connection();
      sw(STATE_CONNECTERROR);
    }
    }
    break;
  case STATE_LOGIN: {
    StringPrintf pass("%s\r\n",passwd);
    connection->send((char *)pass.getValue(),pass.len());
    sw(STATE_LOGINWAIT);
    }
    break;
  case STATE_LOGINWAIT: {
    connection->run();
    char buf[512];
    int l=connection->recv_bytes(buf,sizeof(buf));
    if(l) {
      if(buf[0]=='O' && buf[1]=='K') sw(STATE_SENDHEADER);
      else sw(STATE_LOGINERROR);
    } else if(getTimeout()>10000) {
      close_connection();
      sw(STATE_CONNECT);
    }
    }
    break;
  case STATE_SENDHEADER: {
    String head;
    head+=StringPrintf("icy-name:%s\r\n",name);
    head+=StringPrintf("icy-genre:%s\r\n",genre);
    head+=StringPrintf("icy-url:%s\r\n",url);
    head+=StringPrintf("icy-irc:%s\r\n",irc);
    head+=StringPrintf("icy-icq:%s\r\n",icq);
    head+=StringPrintf("icy-aim:%s\r\n",aim);
    head+=StringPrintf("icy-pub:%i\r\n",pub);
    {
      MediaInfoI mi;
      encoder->getInfos(&mi);
      head+=StringPrintf("icy-br:%i\r\n",mi.getDataInt("bitrate"));
    }
    if(encoder->getContentType()) {
      head+=StringPrintf("Content-Type: %s\r\n",encoder->getContentType());
    }
    head+="\r\n";
    connection->send((char *)head.getValue(),head.len());
    connection->run();
    sw(STATE_SEND);
    }
    break;
  case STATE_SEND: {
    char buf[16384];
    int nbtosend=MIN(to_send.getSize(),connection->send_bytes_available());
    nbtosend=MIN(nbtosend,(int)sizeof(buf));
    if(nbtosend) {
      int s=to_send.get(buf,nbtosend);
      if(s) connection->send(buf,s);
      bytes_sent+=s;
    }
    connection->run();
    int st=connection->get_state();
    if(st==JNL_Connection::STATE_ERROR || st==JNL_Connection::STATE_CLOSED) {
      close_connection();
      sw(STATE_SENDERROR);
    } else {
      run_listener_get();
      run_title_upd();
    }
    }
    break;
  case STATE_CONNECTERROR: 
  case STATE_SENDERROR: {
    if(getTimeout()>=(retrydelay*1000)) sw(STATE_CONNECT);
    }
    break;
  }
}

void Server::close_connection() {
  if(connection) {
    connection->close();
    delete connection;
    connection=NULL;
  }
}

void Server::run_listener_get() {
  switch(listener_get_state) {
  case 0: if((last_listenerget+30000)<GetTickCount()) listener_get_state=1; break;
  case 1: {
    listenersget=new JNL_HTTPGet();
    listenersget->addheader("User-Agent: ShoutcastDSP (Mozilla Compatible)");
    listenersget->connect(StringPrintf("http://%s:%i/",host,port));
    listener_get_state=2;
    }
    break;
  case 2: {
    int r=listenersget->run();
    if(r==-1||r==1) listener_get_state=100;
    if(listenersget->bytes_available()) {
      char buf[16384];
      int s=listenersget->get_bytes(buf,sizeof(buf));
      listener_buffer.add((void *)&buf,s);
    }
    if(r==1) {
      char zero=0;
      listener_buffer.add((void *)&zero,1);
      char *workbuf=(char *)listener_buffer.getBuffer();
      char *tempy = STRSTR(workbuf, "with <B>");  // the nasty crutch that can fuck us
      if ( tempy ) {
			  int retval = tempy - workbuf + 8;
			  tempy += 8;
			  char *tempx = tempy;
			  while ( *tempy  != '<' ) tempy++;
			  *tempy = 0;
        nb_listeners=ATOI(tempx);
  		} else nb_listeners=0;
    }
    }
    break;
  case 100: {
    last_listenerget=GetTickCount();
    delete listenersget;
    listener_get_state=0;
    listener_buffer.reset();
  }
  }
}

void Server::run_title_upd() {
  if(!title_upd) return;
  if(!send_title) return;
  
  switch(titleupd_state) {
  case 0: if((last_titleupd+30000)<GetTickCount()) titleupd_state=1; break;
  case 1: {
    titleupdget=new JNL_HTTPGet();
    titleupdget->addheader("User-Agent:Mozilla");
    String fsongtitle;
    fixstring(songtitle,fsongtitle);
    titleupdget->connect(StringPrintf("http://%s:%i/admin.cgi?pass=%s&mode=updinfo&song=%s&url=%s",host,port,passwd,fsongtitle,""));
    titleupd_state=2;
    }
    break;
  case 2: {
    int r=titleupdget->run();
    if(r==-1||r==1) titleupd_state=100;
    }
    break;
  case 100: {
    last_titleupd=GetTickCount();
    delete titleupdget;
    titleupd_state=0;
    send_title=0;
  }
  }
}

static void lConfig(const char *p, const char *n, String &s) {
  char tmp[1024];
  WASABI_API_CONFIG->getStringPrivate(StringPrintf("%s%s",p,n),tmp,sizeof(tmp),s);
  s=tmp;
}

static void sConfig(const char *p, const char *n, String &s) {
  WASABI_API_CONFIG->setStringPrivate(StringPrintf("%s%s",p,n),s);
}

void Server::loadConfig(const char *pre) {
  lConfig(pre,"name",name);
  lConfig(pre,"genre",genre);
  lConfig(pre,"url",url);
  lConfig(pre,"aim",aim);
  lConfig(pre,"icq",icq);
  lConfig(pre,"irc",irc);
  pub=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s%s",pre,"pub"),pub);
  title_upd=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s%s",pre,"title_upd"),title_upd);
  lConfig(pre,"host",host);
  port=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s%s",pre,"port"),port);
  lConfig(pre,"passwd",passwd);
  autoconnect=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s%s",pre,"autoconnect"),autoconnect);
  int encnum=WASABI_API_CONFIG->getIntPrivate(StringPrintf("%s%s",pre,"encodernum"),encodernum);
  switchEncoder(encnum);
  encoder->loadConfig(pre);
}

void Server::saveConfig(const char *pre) {
  sConfig(pre,"name",name);
  sConfig(pre,"genre",genre);
  sConfig(pre,"url",url);
  sConfig(pre,"aim",aim);
  sConfig(pre,"icq",icq);
  sConfig(pre,"irc",irc);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s%s",pre,"pub"),pub);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s%s",pre,"title_upd"),title_upd);
  sConfig(pre,"host",host);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s%s",pre,"port"),port);
  sConfig(pre,"passwd",passwd);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s%s",pre,"autoconnect"),autoconnect);
  WASABI_API_CONFIG->setIntPrivate(StringPrintf("%s%s",pre,"encodernum"),encodernum);
  encoder->saveConfig(pre);
}

void Server::fixstring(const char *a, String &b) {
  unsigned char *in=(unsigned char *)a;
  unsigned char tmp[1024];
  unsigned char *out=tmp;
  while (*in) {
    if ((*in >= 'A' && *in <= 'Z')||
	  (*in >= 'a' && *in <= 'z')||
	  (*in >= '0' && *in <= '9')) *out++=*in++;
      else {
	      int i=*in;
        unsigned char *t=in;
        in++;
        if (in == t + 1) {
	        if (i == '\'') i = '`';
	        else if (i == '<') i = '(';
	        else if (i == '>') i = ')';
	        else if (i == '\"') i='`';
	        SPRINTF((char *)out,"%%%02X",i);
	        out += 3;
        }
	    }
  }
  *out=0;
  b=(const char *)tmp;
}

void Server::switchEncoder(int newenc) {
  if(newenc==encodernum) return;
  removeEncoderConfig();
  delete(encoder);
  encodernum=newenc;
  encoder=Encoders::createInstance(encodernum);
}
