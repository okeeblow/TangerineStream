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
#ifndef _XUIWAC_H
#define _XUIWAC_H

#include "../studio/wac.h"
class ButtonWnd;

// {4E86EE9B-802E-45af-B76A-D7CA29749651} 
static const GUID cfg_general_guid = 
{ 0x4e86ee9b, 0x802e, 0x45af, { 0xb7, 0x6a, 0xd7, 0xca, 0x29, 0x74, 0x96, 0x51 } };

// {EDAA0599-3E43-4eb5-A65D-C0A0484240E7}
static const GUID cfg_audio_guid = 
{ 0xedaa0599, 0x3e43, 0x4eb5, { 0xa6, 0x5d, 0xc0, 0xa0, 0x48, 0x42, 0x40, 0xe7 } };

// {8BA77FAC-D8FC-4d3e-A134-A99296D4A86B}
static const GUID cfg_media_guid = 
{ 0x8ba77fac, 0xd8fc, 0x4d3e, { 0xa1, 0x34, 0xa9, 0x92, 0x96, 0xd4, 0xa8, 0x6b } };

// {BE15DB34-8A5A-4ddc-A17B-90458438B3E6}
static const GUID cfg_visual_guid = 
{ 0xbe15db34, 0x8a5a, 0x4ddc, { 0xa1, 0x7b, 0x90, 0x45, 0x84, 0x38, 0xb3, 0xe6 } };

// {F8E977B4-7122-4e45-8B32-1EF5312F707E}
static const GUID cfg_internet_guid = 
{ 0xf8e977b4, 0x7122, 0x4e45, { 0x8b, 0x32, 0x1e, 0xf5, 0x31, 0x2f, 0x70, 0x7e } };

// {C7F1F62E-D6E0-4af3-8CD5-0BD2A3529615}
static const GUID cfg_display_guid = 
{ 0xc7f1f62e, 0xd6e0, 0x4af3, { 0x8c, 0xd5, 0xb, 0xd2, 0xa3, 0x52, 0x96, 0x15 } };

// {493448C8-A701-4eca-978D-C17DAAD3C0FA}
static const GUID cfg_advanced_guid= 
{ 0x493448c8, 0xa701, 0x4eca, { 0x97, 0x8d, 0xc1, 0x7d, 0xaa, 0xd3, 0xc0, 0xfa } };

// {854C5196-8F3E-46ea-987D-9DC2A5B9DD46}
static const GUID cfg_vis_guid = 
{ 0x854c5196, 0x8f3e, 0x46ea, { 0x98, 0x7d, 0x9d, 0xc2, 0xa5, 0xb9, 0xdd, 0x46 } };

// {38863227-9891-408c-8DB2-9EDE158418AB}
static const GUID cfg_fades_guid = 
{ 0x38863227, 0x9891, 0x408c, { 0x8d, 0xb2, 0x9e, 0xde, 0x15, 0x84, 0x18, 0xab } };


// {B397A4CE-455A-4d62-8BF6-D0F91ACB70E6} // The WAC's GUID (also its window's GUID, because by convention, a WAC that implements a 'main' window uses its own GUID to expose it).
static const GUID preferences_guid = 
{ 0xb397a4ce, 0x455a, 0x4d62, { 0x8b, 0xf6, 0xd0, 0xf9, 0x1a, 0xcb, 0x70, 0xe6 } };

class MainWindow {
  public:
  static const char *getWindowGroup() { return "wasabi.preferences.group"; }
  static const char *getWindowName() { return "Preferences"; }
  static GUID getWindowGuid() { return preferences_guid; }
};

class WACPreferences : public WAComponentClient {
public:
  WACPreferences();

  virtual const char *getName() { return MainWindow::getWindowName(); };
  static const char *getWindowTypeName() { return MainWindow::getWindowName(); }
  static GUID getWindowTypeGuid() { return MainWindow::getWindowGuid(); }
  virtual GUID getGUID();
  static void setIconBitmaps(ButtonWnd *button);

  virtual void onCreate();
  virtual void onDestroy();

  void openGroup(const char *group);

private:

};

extern int disallow_select_default;

extern WACPreferences *preferenceswac;

#endif
