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
#ifndef __QUERYDRAG_H
#define __QUERYDRAG_H

#include "../common/guiobjwnd.h"
#include "../bfc/autobitmap.h"

class FilenameI;

#define  QUERYDRAG_PARENT GuiObjectWnd 

// -----------------------------------------------------------------------
// Your wnd object class

class QueryDrag : public  QUERYDRAG_PARENT {
  
  public:

    QueryDrag();
    virtual ~QueryDrag();

    virtual int onPaint(Canvas *c); 
    virtual int getPreferences(int what); 
    virtual int onMouseMove(int x, int y);

    virtual int setXuiParam(int xuihandle, int xmlattributeid, const char *xmlattributename, const char *value);

    void setImage(const char *elementname);
    void setSource(const char *elementname);
    void onBeginDrag();
    virtual int dragComplete(int success);

  private:

    AutoSkinBitmap image;
    String source;

    FilenameI *fn;

    enum {
       QUERYDRAG_SETIMAGE = 0,
       QUERYDRAG_SETSOURCE,
    };
    int myxuihandle;
};


// -----------------------------------------------------------------------
extern char QueryDragXuiObjectStr[];
extern char QueryDragXuiSvcName[];
class QueryDragXuiSvc : public XuiObjectSvc<QueryDrag, QueryDragXuiObjectStr, QueryDragXuiSvcName> {};

#endif
