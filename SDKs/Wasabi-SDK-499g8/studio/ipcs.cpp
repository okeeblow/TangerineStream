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
#include "main.h"
#include "ipcs.h"
#include "linux.h"

using namespace IpcsCommand;

#ifdef WIN32
IpcsPtr::IpcsPtr(HWND h) {
  hwnd = h;
}
#else
IpcsPtr::IpcsPtr(int q) {
  qid = q;
}
#endif

void IpcsPtr::moveToForeground() {
#ifdef WIN32
  if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
//  ShowWindow(hwnd,SW_SHOW); //FG> SW_RESTORE should take care of it and is trapped for taskbar button hiding. Explicitly showing the window will make an iconless button reapear if studio is set to not have a taskbar button
  SetForegroundWindow(hwnd);
#else
  DebugString( "portme -- IpcsPtr::moveToForeground\n" );
#endif
}

void IpcsPtr::sendWasabiCommand(int command, void *param, int paramlen) {
#ifdef WIN32
  COPYDATASTRUCT cd;
  cd.dwData=command;
  cd.cbData=paramlen;
  cd.lpData=param;
  SendMessage(hwnd, WM_COPYDATA, NULL, (long)&cd);
#else
  wa_msgbuf msg;
  msg.mtype = command;
  ASSERT( paramlen < IPC_MSGMAX - 4 );
  msg.paramlen = paramlen;
  MEMCPY( msg.param, param, paramlen );

  if ( msgsnd( qid , &msg, IPC_MSGMAX, 0 ) == 1 ) {
    perror( "msgsnd" );
  }
#endif
}

void IpcsPtr::sendWasabiCommand(int command, const char *param) {
  sendWasabiCommand(command, (void *)param, STRLEN(param)+1);
}


IpcsPtr *Ipcs::getOtherWasabiInstance() {
  extern String ipcWindowClassName;

#ifdef WIN32
  HWND hwnd_instance=FindWindow(ipcWindowClassName,NULL);
  if(!hwnd_instance) return NULL;
  
  return(new IpcsPtr(hwnd_instance));
#else
  int key = ftok( ".", 'w' );

  int qid = msgget( key, 0 );
  if ( qid == -1 && errno == ENOENT ) {
    qid = msgget( key, IPC_CREAT | IPC_EXCL | 0660 );

    LinuxAPI::setIPCId( qid );

    return NULL;
  } else if ( qid == -1 ) {
    return NULL;
  } else {
    return new IpcsPtr( qid );
  }
#endif
}

int Ipcs::onIpcsMessage(int command, void *param, int paramlen) {
  switch(command) {
  case IPC_COMMANDLINE:
    Main::processCommandLine((const char *)param);
    return 0;
  }
  return 0;
}
