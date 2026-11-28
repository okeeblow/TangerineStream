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
#ifndef __SCANNERSERVER_H
#define __SCANNERSERVER_H

#include "../timerclient.h"
#include "../ptrlist.h"
#include "../../studio/runlevelcb.h"

class SharedDbScannerI;

class ScannerServer {
  public:

    ScannerServer();
    virtual ~ScannerServer();
      
    virtual void scannerserver_newScanner();
    virtual void scannerserver_setTable(GUID table);
    virtual GUID scannerserver_getTable() { return table_guid; }
    virtual SharedDbScannerI *scannerserver_getScanner();

    virtual void scannerserver_onDropScanner(SharedDbScannerI *scanner) {  } 
    virtual void scannerserver_onNewScanner(SharedDbScannerI *scanner) {  } 

    virtual Dependent *scannerserver_getDependencyPtr()=0;

  private:

    void scheduleScannerDeletion(SharedDbScannerI *scan);

    SharedDbScannerI *scanner;
    GUID table_guid;
};

class DeferedScannerDeleter : public TimerClientDI {
  public:
    DeferedScannerDeleter() { timerset = 0; }  
    virtual ~DeferedScannerDeleter() { }

    void scheduleDeletion(SharedDbScannerI *s);
    void runlevelcb_onShutdown();
    void timerclient_timerCallback(int id);

  private:
    PtrList<SharedDbScannerI> deletingscanners;
    int timerset;
};

extern DeferedScannerDeleter *scannerdeleter;

#endif