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
#include "httpread.h"
#include "reader.h"

static WACNAME wac;
WAComponentClient *the = &wac;

// {AE04FB30-53F5-4032-BD29-032B87EC3404}
static const GUID guid = 
{ 0xbc10fa00, 0x53f5, 0x4032, { 0xa0, 0x09, 0x2, 0x2b, 0x87, 0xec, 0x34, 0x04 } };

_bool config_guess_prebuffer("Automatic prebuffer size", true);
_string config_proxy("Proxy server");
_bool config_proxy80("Only use proxy on port 80",false);
_int config_buffer_size("Buffer size (in Kb)",64);
_int config_prebuffer_size("Prebuffer size (in Kb)",24);
_int config_prebuffer_min("Buffer underrun limit (in Kb)",0); // ern changed from 2 to 0
_bool config_allowseek("Allow seeking on HTTP files",true);
_bool config_fullseek("Fully seekable HTTP files (can be slow)",true);
_int config_seekprebuffer("Prebuffer size on seek (in Kb)",1);
_bool config_suppressstatus("Suppress buffering status messages", FALSE);

WACNAME::WACNAME() {
  registerService(new waServiceT<svc_fileReader, HttpRead>);
  registerAttribute(&config_guess_prebuffer);
  registerAttribute(&config_proxy);
  registerAttribute(&config_proxy80);
  registerAttribute(&config_buffer_size);
  registerAttribute(&config_prebuffer_size);
  registerAttribute(&config_prebuffer_min);
  registerAttribute(&config_allowseek);
  registerAttribute(&config_fullseek);
  registerAttribute(&config_seekprebuffer);
  registerAttribute(&config_suppressstatus);


// {F8E977B4-7122-4e45-8B32-1EF5312F707E}
  const GUID cfg_internet_guid = 
{ 0xf8e977b4, 0x7122, 0x4e45, { 0x8b, 0x32, 0x1e, 0xf5, 0x31, 0x2f, 0x70, 0x7e } };
// {4E5C44BA-9EAD-4b69-89B1-6954DD040C53}
  const GUID cfg_http_guid = 
{ 0x4e5c44ba, 0x9ead, 0x4b69, { 0x89, 0xb1, 0x69, 0x54, 0xdd, 0x4, 0xc, 0x53 } };
  registerPrefGroup("winamp.preferences.httpreader", "HTTP Reader", cfg_http_guid, cfg_internet_guid);
}

WACNAME::~WACNAME() {
}

GUID WACNAME::getGUID() {
  return guid;
}

void WACNAME::onRegisterServices() {
//CUT 
//  config_guess_prebuffer.setName("Automatic prebuffer size");
//  config_guess_prebuffer=true;
//  registerAttribute(&config_guess_prebuffer);
  
//  config_proxy.setName("Proxy server");
//  config_proxy="";
//  registerAttribute(&config_proxy);
}
