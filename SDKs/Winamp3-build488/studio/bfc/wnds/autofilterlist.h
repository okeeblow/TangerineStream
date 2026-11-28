/*

  Nullsoft WASABI Source File License

  Copyright 1999-2001 Nullsoft, Inc.

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


  Brennan Underwood
  brennan@nullsoft.com

*/

#ifndef __AUTOFILTERLIST_H
#define __AUTOFILTERLIST_H

#include "../std.h"
#include "../../common/listwnd.h"
#include "../db/scanner.h"
#include "../timeslicer.h"
#include "../db/subqueryserver.h"
#include "../db/multiqueryclient.h"
#include "autoquerylist.h"

/**
  class FilterListItem represents a data item of any type or 
  size to put on a FilterList. The type and size are accessible 
  for memory management purposes.
  
  @short Items for FilterLists
  @author Nullsoft
  @ver 1.0
  @see FilterListItemSort, MultiQueryServer
  @cat BFC
*/
class FilterListItem {
  public:
    
    /**
      FilterListItem constructor allocates memory for copying a data 
	    object, and then copies the object into newly allocated memory. 
    
      @see MemBlock, VoidMemblock
      @ret Constructor
      @param _data      Pointer to a data object
      @param _datalen   Length of data object
      @param _datatype  Type of data object
    */
    FilterListItem(void *_data, int _datalen, int _datatype);
    
    /**
      FilterListItem destructor does nothing. Destruction is handled in ??.

      @see 
      @ret Destructor
      @param None
    */
    virtual ~FilterListItem() {  }

    /**
      FilterListItem method getMemory queries the size of the memory 
	  block holding the object data.
    
      @see 
      @ret Size of data
      @param None
    */
    const char *getData() { return data.getMemory(); }
    
    /**
      FilterListItem method getDatatype queries the data type.
    
      @see 
      @ret Data type of item
      @param None
    */
    int getDatatype() { return data_type; }

    /**
      FilterListItem method getDataLen queries the length of the data.

      @see 
      @ret Length of data
      @except 
      @param None
    */
    int getDataLen() { return data_len; }

  private:
  
    MemBlock<char> data;
    int data_type;
    int data_len;
};

/**
  Class FilterListItemSort has no constructor or destructor. 
  It provides methods for sorting a FilterList.
  
  @short FilterList sorting.
  @author Nullsoft
  @ver 1.0
  @see FilterListItem
  @cat BFC
*/
class FilterListItemSort {
  public:
    /**
      FilterListItemSort method compareItem determines the data 
	    types of two objects and calls an appropriate comparison
	    function.
    
      @assert The two objects to be compared have the same data type.
      @see compareAttrib()
      @ret -1, _p1 < _p2; 0, _p1 = _p2; 1, _p1 > _p2
      @param _p1 Object to compare
      @param _p2 Object to compare
    */
    static int compareItem(void *_p1, void *_p2) {
      FilterListItem *p1 = ((FilterListItem *)_p1);
      FilterListItem *p2 = ((FilterListItem *)_p2);
      ASSERT(p1->getDatatype() == p2->getDatatype());
      switch (p1->getDatatype()) {
        case MDT_INT: 
        case MDT_TIME:
        case MDT_BOOLEAN:
        case MDT_TIMESTAMP: {
          int a = *(int *)p1->getData();
          int b = *(int *)p2->getData();
          if (a < b) return -1;
          if (a > b) return 1;
          return 0; }
        case MDT_STRINGZ:
          return STRICMP(p1->getData(), p2->getData());
      }
      return 0;
    }
    
    /**
      FilterListItemSort method compareAttrib compares the value 
	  of an item to an attribute string, or to the value of the 
	  attribute string cast to an appropriate data type.
    
      @see compareItem()
      @ret -1, _p1 < _p2; 0, _p1 = _p2; 1, _p1 > _p2
      @param attrib
      @param _item
    */
    static int compareAttrib(const char *attrib, void *_item) {
      FilterListItem *item = ((FilterListItem *)_item);
      switch (item->getDatatype()) {
        case MDT_INT: 
        case MDT_TIME:
        case MDT_BOOLEAN:
        case MDT_TIMESTAMP: {
          int a = *(int *)attrib;
          int b = *(int *)item->getData();
          if (a < b) return -1;
          if (a > b) return 1;
          return 0; }
        case MDT_STRINGZ:
          return STRICMP(attrib, item->getData());
      }
      return 0;
    }
};

class ButtHooker;
class FilenameI;

#define AUTOFILTERLIST_PARENT ListWnd
#define AUTOFILTERLIST_DBPARENTSRV SubQueryServerI
#define AUTOFILTERLIST_DBPARENTCLIENT MultiQueryClientI

/**
  Class AutoFilterList represents a list of items to filter on.
  
  @short A list of items to filter on.
  @author Nullsoft
  @ver 1.0
  @see FilterListItemSort
  @see FilterListItem
  @cat BFC
*/
class AutoFilterList : public AUTOFILTERLIST_PARENT, 
                       public AUTOFILTERLIST_DBPARENTSRV, 
                       public AUTOFILTERLIST_DBPARENTCLIENT {
  public:

    /**
      AutoFilterList constructor creates an empty filter list with 
	  a NULL local scanner.
    
      @see ~AutoFilterList()
      @ret Constructor
      @param None
    */
    AutoFilterList();
    
    /**
      AutoFilterList destructor deletes the local scanner.

      @see AutoFilterList()
      @ret Destructor
      @param None
    */
    virtual ~AutoFilterList();

    /**
      AutoFilterList method setMetadataField sets the field name 
	  and sets the scanner to NULL.
    
      @see 
      @ret None
      @param _field The name of the field.
    */
    void setMetadataField(const char *field);

    /**
      AutoFilterList method onInit creates and populates the list.
      
      @see 
      @ret 1
      @param None
    */
    virtual int onInit();
    
    /**
      AutoFilterList method onResize prepares a window for 
	  repainting in a new size.

      @see 
      @ret 1
      @param None
    */
    virtual int onResize();

    virtual void getClientRect(RECT *);
    virtual void rootwndholder_getRect(RECT *r);
    virtual void onNewContent();
    
    /**
      AutoFilterList method ownerDraw displays a window in a 
	  specified position and state.

      @see 
      @ret 1
      @param canvas
      @param pos
      @param r
      @param lParam
      @param isselected
      @param isfocused
    */
    virtual int ownerDraw(Canvas *canvas, int pos, RECT *r, LPARAM lParam, int isselected, int isfocused);
    virtual void onLeftClick(int itemnum);	
    virtual void onDoubleClick(int itemnum);	// double-click on an item

    /**
      AutoFilterList method timeslicer_getDependencyPtr gets the dependency pointer.
    
      @ret The dependency pointer.
      @param None.
    */
    virtual Dependent *timeslicer_getDependencyPtr() { return rootwnd_getDependencyPtr(); }

    /**
      AutoFilterList method getAllString returns the string "All".
      
      @see 
      @ret "All"
      @param None
    */
    virtual const char *getAllString() { return "All"; }


    /**
      AutoFilterList method scannerserver_onNewScanner deletes 
	  the current scanner and sets up a new one.
      
      @see 
      @ret None
      @param A shared database scanner
    */
    virtual void scannerserver_onNewScanner(SharedDbScannerI *scanner);

    /**
      AutoFilterList method mqc_onNewMultiQuery makes the 
	  necessary preparations for running a new query.
      
      @see MultiQueryServer, SubQueryServer
      @ret None
      @param modifier refers to a SubQueryServer, which 
	  implements a simple filter.
    */
    virtual void mqc_onNewMultiQuery(SubQueryServer *modifier, int flag);
    
    /**
      AutoFilterList method mqc_getDependencyPtr gets the dependency pointer.
      
      @see Dependent
      @ret Dependency pointer
      @param None
    */
    virtual Dependent *mqc_getDependencyPtr() { return rootwnd_getDependencyPtr(); }

    /**
      AutoFilterList method setOrder sets the order number.
      
      @see 
      @ret None
      @param Order number
    */
    void setOrder(int n) { order = n; }

    /**
      AutoFilterList method sqs_getCooperativeId queries the order number.
      
      @see 
      @ret Order number if linked, otherwise -1
      @param None
    */
    virtual int sqs_getCooperativeId() { return linked ? order : -1; }

    /**
      AutoFilterList method mqc_onCompleteMultiQuery has no 
	  external effect.
      
      @see 
      @ret None
      @param None
    */
    virtual void mqc_onCompleteMultiQuery();

    /**
      AutoFilterList method mqc_onAddPlaystring tests whether 
	  to enter a playstring in a filter entry, and does it if
	  necessary.
      
      @see 
      @ret None
      @param None
    */
    virtual void mqc_onAddPlaystring(const char *playstring, int nitems, int thispos);


    /**
      AutoFilterList method sqs_onAttachServer registers a 
	  client with a query server.
      
      @see 
      @ret None
      @param None
    */
    virtual void sqs_onAttachServer(MultiQueryServer *s);

    /**
      AutoFilterList method sqs_onDetachServer unregisters a 
	  client with a query server.
      
      @see 
      @ret None
      @param None
    */
    virtual void sqs_onDetachServer(MultiQueryServer *s);

    virtual void sqs_reset();
    virtual int onDeferredCallback(int p1, int p2);

    /**
      AutoFilterList method setLinked sets the linking status
	  according to the value v.
      
      @see 
      @ret None
      @param v linking status to set
    */
    virtual void setLinked(int v) { linked = v; }

    void setQueryDirection(int glag);

    /**
      
    */
    void doFieldPopup();
    
    /**
      
    */
    virtual void onVScrollToggle(BOOL set);
    
    /**
      
    */
    virtual int wantRenderBaseTexture() { return 1; }

    virtual int onBeginDrag(int iItem);
    virtual int dragComplete(int success);

  private:
    /**
      
    */
    void populate();
    
    /**
      
    */
    void filterEntry(const char *playstring, dbScanner *scanner, const char *field);
    
    /**
      
    */
    void filterInt(int data);
    
    /**
      
    */
    void filterString(const char *data);
    
    /**
      
    */
    void insertData(void *data, int len, int type);
    
    /**
      
    */
    void setLabelName();

    String field;
    dbScanner *local_scanner;
    PtrListInsertSorted<FilterListItem, FilterListItemSort> uniques;
    int data_type;
    int need_startslicer;
    int order;
    int grab_playstrings;
    int viscount;
    int linked;
    int needrestart;
    int querydirection;
    int last_populate_flag;
  ButtHooker *hooker;
  FilenameI *fn;
};

#endif
