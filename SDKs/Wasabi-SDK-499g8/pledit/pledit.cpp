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

#include "pledit.h"
#include "plitem.h"
#include "plstatus.h"
#include "playlisti.h"
#include "defdrop.h"

#include "../studio/assert.h"
#include "../studio/api.h"

#include "resource.h"

#include "../bfc/wnds/typesheet.h"
#include "../common/metatags.h"
#include "../common/metadatasvc.h"
#include "../common/nsguid.h"
#include <core/corehandle.h>

#include <bfc/attribs/attribs.h>

#include "../studio/services/creators.h"
#include "../bfc/wndcreator.h"
#include "pldirobj.h"

#include "pldirsvc.h"	// static PlDir service
#include "pldirwnd.h"	// class PlDirWnd, is a view on the PlDir svc
#include "editori.h"
#include "selwnd.h"
#include "plcmdline.h"

#include "pldir.h"	// playlist dir as object directory

WACpledit wacpledit;
#ifndef STATIC_COMPONENT
WAComponentClient *the = &wacpledit;
#endif

_bool shuffle("Shuffle");
_int repeat("Repeat");
//CUT_bool stop_on_each("Stop on each", FALSE);
_bool show_numbers("Show numbers in playlist", TRUE);
//_bool nofadesequential;
_int shuffle_window("Shuffle window size", 33);
_int shuffle_randomize("Shuffle on next chance", 100);
_bool enqueue("Enqueue as default action", FALSE);
_string newplname("New playlist name", "Playlist %03d");
_bool incpl("Include playlists when adding directory", FALSE);
_bool http_recurse("Recurse in HTTP playlists", FALSE);
_string def_columns("Playlist editor columns", "");

_int poppler_maxwidth("Poppler max width", 100);
_int poppler_minwidth("Poppler min width", 130);
_int poppler_maxheight("Poppler max height", 30);
_int poppler_minheight("Poppler min height", 40);
_bool poppler_customsnap("Enable custom snap-points", FALSE);

#include "../studio/services/servicei.h"

// {45F3F7C1-A6F3-4ee6-A15E-125E92FC3F8D}
GUID component_guid = 
{ 0x45f3f7c1, 0xa6f3, 0x4ee6, { 0xa1, 0x5e, 0x12, 0x5e, 0x92, 0xfc, 0x3f, 0x8d } };

WACpledit::WACpledit() : WAComponentClient("Playlist Editor") {

  pldir_registerServices(this);

  // playlist directory service
  registerService(new waServiceFactoryTSingle<svc_plDir, PlDirSvc>(svc_plDir::getServiceGuid()));
  // playlist directory as a drop target
  registerService(new DropTargetCreator<DefaultDrop>);
  // playlist directory script object
  registerService(new ScriptObjectCreator<PlDirScriptObjectSvc>);
  // command line callback for pldir
  registerResource(new PlCmdLine);
  // playlist directory view
  registerService(new waServiceFactoryTSingle< svc_windowCreate, CreateWndByType<PlDirWnd> >(new CreateWndByType<PlDirWnd>("plsc")));
  // selection window
  registerService(new waServiceFactoryTSingle< svc_windowCreate, CreateWndByType<SelWnd> >(new CreateWndByType<SelWnd>("plsc")));

  // playlist editor itself
  registerService(new XuiObjectCreator<EditorXuiSvc>);

  // private db table
  MetaDataSvc *mds = new MetaDataSvc("Playlist Editor", nsGUID::fromChar("{37076A53-7A78-4dcc-AF91-374700C00009}"));
  mds->addColumn(MT_SHUFFLEID, MDT_INT, FALSE, FALSE);
  registerService(new MetaDataSvcFactory(mds));

  // prefs page
// {4E86EE9B-802E-45af-B76A-D7CA29749651} 
  const GUID cfg_general_guid = 
{ 0x4e86ee9b, 0x802e, 0x45af, { 0xb7, 0x6a, 0xd7, 0xca, 0x29, 0x74, 0x96, 0x51 } };
// {5AECAC95-80BC-475f-908C-9C49945A3B07}
  const GUID cfg_pl_guid = 
  { 0x5aecac95, 0x80bc, 0x475f, { 0x90, 0x8c, 0x9c, 0x49, 0x94, 0x5a, 0x3b, 0x7 } };
  registerPrefGroup("winamp.preferences.pl", "Playlists", cfg_pl_guid, cfg_general_guid);
// {9DAB72DD-CB12-49e4-98EC-691576897021}
const GUID cfg_pladv_guid = 
{ 0x9dab72dd, 0xcb12, 0x49e4, { 0x98, 0xec, 0x69, 0x15, 0x76, 0x89, 0x70, 0x21 } };

  registerPrefGroup("winamp.preferences.pl.advanced", "Advanced", cfg_pladv_guid, cfg_pl_guid);

  PlaylistI::registerServices(this);
  PlDirWnd::registerServices(this);

  registerExtension("*.m3u;*.b4s;*.pls", "Playlist files", "Playlist");

  registerAttribute(&shuffle);
  registerAttribute(&shuffle_window);
  registerAttribute(&shuffle_randomize);
  registerAttribute(&repeat);
  registerAttribute(&show_numbers);
  registerAttribute(&newplname);
  registerAttribute(&enqueue);
  registerAttribute(&incpl);
  registerAttribute(&http_recurse);
  registerAttribute(&def_columns);

  registerAttribute(&poppler_maxwidth);
  registerAttribute(&poppler_minwidth);
  registerAttribute(&poppler_maxheight);
  registerAttribute(&poppler_minheight);
  registerAttribute(&poppler_customsnap);
}

WACpledit::~WACpledit() {
}

GUID WACpledit::getGUID() {
  return component_guid;
}
