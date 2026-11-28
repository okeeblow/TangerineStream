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
#include "library.h"
#include "xuiaddmusic.h"
#include "../studio/services/creators.h"
#include "querydir.h"
#include "querydircmd.h"
#include "../bfc/wndcreator.h"
#include "resource.h"
#include "reloadinfocmd.h"
#include "../bfc/wnds/buttwnd.h"
#include "../bfc/wndcreator.h"

#include "delitem.h"


static WAClibrary Library;
WAComponentClient *the = &Library;
WAClibrary *xuiobjectwac = &Library;

extern waServiceFactoryI *feedolio;

// {6B0EDF80-C9A5-11d3-9F26-00C04F39FFC6}
const GUID library_guid = { 0x6b0edf80, 0xc9a5, 0x11d3, { 0x9f, 0x26, 0x0, 0xc0, 0x4f, 0x39, 0xff, 0xc6 } };

WAClibrary::WAClibrary() : WAComponentClient("Media Library") {
  registerService(new XuiObjectCreator<AddMusicXuiSvc>);//"add music" button
  registerService(feedolio);

  registerService(new ObjectDirCreator<QueryDir>);// query directory
  registerService(new ContextCmdCreator<QdCmds>);

  // some context commands
  registerService(new ContextCmdCreator<ReloadInfoCmd>);
  registerService(new ContextCmdCreator<DelItemCmd>);
}

GUID WAClibrary::getGUID() {
  return library_guid;
}
