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
#ifndef _SVC_PLAYER_H
#define _SVC_PLAYER_H

#include "../bfc/dispatch.h"

#include "../studio/services/services.h"

class RootWnd;

class svc_player {
public:
  static const char *getServiceName() { return "Standard player service"; }
  static int getServiceType() { return WaSvc::UNIQUE; }
  static GUID getServiceGuid() {
    // {9DE79C4A-B9E6-46f4-9AE2-40E0F09CCA51}
    const GUID guid = 
    { 0x9de79c4a, 0xb9e6, 0x46f4, { 0x9a, 0xe2, 0x40, 0xe0, 0xf0, 0x9c, 0xca, 0x51 } };
    return guid;
  }
  virtual void openFile(const char *filename, FOURCC droptarg=MK3CC('d','e','f'))=0;
  virtual void openFiles(RootWnd *parent=NULL, const char *type=NULL, FOURCC droptarg=MK3CC('d','e','f'))=0;
};

class svc_playerI : public svc_player {
};

#include "../studio/services/servicei.h"

template <class T>
class PlayerCreator : public waServiceFactoryTSingle<svc_player, T> {
public:
  PlayerCreator() : waServiceFactoryTSingle<svc_player, T>(svc_player::getServiceGuid()) {}
};

#endif
