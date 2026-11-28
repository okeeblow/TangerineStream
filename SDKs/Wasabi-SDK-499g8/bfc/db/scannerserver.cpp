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
#include "scannerserver.h"
#include "sharedscanner.h"
#include "../std.h"

#define TIMER_SCANNERDEL 0x2749

DeferedScannerDeleter *scannerdeleter = NULL;

ScannerServer::ScannerServer(){
  if (scannerdeleter == NULL) {
    scannerdeleter = new DeferedScannerDeleter;
  }
  table_guid = INVALID_GUID;
  scanner = NULL;
  scannerserver_setTable(WASABI_API_APP->main_getGUID());
}

ScannerServer::~ScannerServer() {
}
     
void ScannerServer::scannerserver_newScanner(){
  String query;
  if (scanner != NULL) {
    query = scanner->getQuery();
    scheduleScannerDeletion(scanner);
    scannerserver_onDropScanner(scanner);
    scanner = NULL;
  }
  if (table_guid != INVALID_GUID) {
    scanner = new SharedDbScannerI(table_guid);
    if (!query.isempty()) scanner->setQuery(query);
    scannerserver_onNewScanner(scanner);
  }
}

void ScannerServer::scannerserver_setTable(GUID table){
  if (table_guid == table) return;
  table_guid = table;
  scannerserver_newScanner();  
}

SharedDbScannerI *ScannerServer::scannerserver_getScanner() {
  return scanner;
}

void ScannerServer::scheduleScannerDeletion(SharedDbScannerI *scan) {
  if (!scan) return;
  ASSERT(scannerdeleter != NULL);
  scannerdeleter->scheduleDeletion(scan);
}

void DeferedScannerDeleter::scheduleDeletion(SharedDbScannerI *s) {
  deletingscanners.addItem(s);
  if (!timerset) {
    timerclient_setTimer(TIMER_SCANNERDEL, 500);
    timerset=1;
  }
}

void DeferedScannerDeleter::timerclient_timerCallback(int id) {
  if (id == TIMER_SCANNERDEL) {
    for (int i=0;i<deletingscanners.getNumItems();i++) {
      SharedDbScanner *scan = deletingscanners.enumItem(i);
      if (scan->shareddbscanner_getRefCount() == 0) {
        deletingscanners.removeByPos(i);
        delete scan;
        i--;
      }
    } 
    if (deletingscanners.getNumItems() == 0) {
      timerclient_killTimer(TIMER_SCANNERDEL);
      timerset = 0;
    }
    if (!timerset) {
      scannerdeleter = NULL;
      delete this; // w00t
    }
    return;
  }
  TimerClientDI::timerclient_timerCallback(id);
}



