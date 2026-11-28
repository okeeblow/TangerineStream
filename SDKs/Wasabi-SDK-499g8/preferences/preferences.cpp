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
#include "preferences.h"
#include "xuipreftree.h"

#include "resource.h"
#include "../studio/api.h"
#include "../studio/services/creators.h"
#include "../bfc/wndcreator.h"
#include "cfg_actions.h"
#include "../bfc/wnds/buttwnd.h"

static WACPreferences preferences;
WAComponentClient *the = &preferences;
WACPreferences *preferenceswac = &preferences;

int disallow_select_default=0;

// Our component constructor, register various things.
// REMEMBER, accessing the api pointer here is forbiden, use onCreate for that.
WACPreferences::WACPreferences() {

  // register the Preferences:Tree object
  registerService(new XuiObjectCreator<PreferencesTreeXuiSvc>);

  // register the preferences window xml
  registerSkinFile("xml/preferences.xml", true); 

  // expose our main window group to the system as a GUID window
  registerService(new WndCreateCreatorSingle< CreateWndByGuid< GroupGuidWnd<MainWindow> > >);
  // tell the context menu to show an entry for us
  registerAutoPopup(MainWindow::getWindowGuid(), MainWindow::getWindowName(), "static");
  registerService(new WndCreateCreatorSingle< CreateBucketItem<WACPreferences> >);

  registerService(new ActionCreator<ConfigActionsSvc>);
}

GUID WACPreferences::getGUID() {
  return preferences_guid;
}

void WACPreferences::onCreate() {
  // register a whole bunch of preferences pages
  api->preferences_registerGroup("winamp.preferences.general", "System", cfg_general_guid);
  api->preferences_registerGroup("winamp.preferences.internet", "Internet", cfg_internet_guid);
  api->preferences_registerGroup("winamp.preferences.audio", "Audio", cfg_audio_guid);

  api->preferences_registerGroup("winamp.preferences.media", "Media", cfg_media_guid, cfg_general_guid);
//this one isn't very clear on what it's supposed to be  api->preferences_registerGroup("winamp.preferences.visual", "Visual", cfg_visual_guid, cfg_display_guid);
  api->preferences_registerGroup("winamp.preferences.display", "Display", cfg_display_guid, cfg_general_guid);
  api->preferences_registerGroup("winamp.preferences.advanced", "Advanced", cfg_advanced_guid, cfg_general_guid);
  api->preferences_registerGroup("winamp.preferences.vis", "Visualizations", cfg_vis_guid, /*cfg_visual_guid*/cfg_general_guid);
  //api->preferences_registerGroup("winamp.preferences.fade", "Fadings", cfg_fades_guid, cfg_audio_guid);
}

void WACPreferences::onDestroy() {
}

void WACPreferences::setIconBitmaps(ButtonWnd *button) {
  button->setBitmaps(preferenceswac->gethInstance(), IDB_TAB_NORMAL, NULL, IDB_TAB_HILITED, IDB_TAB_SELECTED);
}

void WACPreferences::openGroup(const char *group) {
  disallow_select_default = 1;
  RootWnd *w = api->skinwnd_enumByGuid(preferences_guid, 0);
  if (w == NULL)
    w = api->skinwnd_createByGuid(preferences_guid);
  if (w != NULL) {
    GuiObject *obj = w->getGuiObject();
    if (obj != NULL) {
      GuiObject *tree = obj->guiobject_findObjectByInterface(prefTreeGuid);
      PreferencesTree *ptree = static_cast<PreferencesTree *>(tree->guiobject_getScriptObject()->vcpu_getInterface(prefTreeGuid));
      ptree->openGroup(group, 1);
    }
  }
  disallow_select_default = 0;
}
