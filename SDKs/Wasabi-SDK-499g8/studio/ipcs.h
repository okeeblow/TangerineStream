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
#ifndef _IPCS_H
#define _IPCS_H

// same value as winamp 2.x
#define WM_WA_IPC WM_USER

#ifdef LINUX
// Stolen from a webpage, I think Linux's is higher, but to be safe...
#define IPC_MSGMAX  4056

struct wa_msgbuf {
  long mtype;
  int paramlen;
  char param[IPC_MSGMAX - 4];
};
#endif

namespace IpcsCommand {
  enum {
    // wa2.x IPC messages
    IPC_PLAYFILE=100,


    // wa3 IPC messages
    IPC_COMMANDLINE=1000,
  };
};

class IpcsPtr {
public:
#ifdef WIN32
  IpcsPtr(HWND hwnd);
#else
  IpcsPtr(int qid);
#endif

  void sendWasabiCommand(int command, void *param=0, int paramlen=0);
  void sendWasabiCommand(int command, const char *param);
  
  void moveToForeground();

private:
#ifdef WIN32
  HWND hwnd;
#else
  int qid;
#endif
};

class Ipcs {
public:
  static IpcsPtr *getOtherWasabiInstance();

  static int onIpcsMessage(int command, void *param, int paramlen);
};

#endif
