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
#ifndef __CURSOR_H
#define __CURSOR_H

#include "dispatch.h"
#include "../studio/skincb.h"
#include "string.h"

class COMEXP Cursor : public Dispatchable {
  
  public:

    OSCURSORHANDLE getOSHandle();

  enum {
    CURSOR_GETOSHANDLE = 0,
  };

};

inline OSCURSORHANDLE Cursor::getOSHandle() {
  return _call(CURSOR_GETOSHANDLE, (OSCURSORHANDLE)NULL);
}

class COMEXP CursorI : public Cursor {

  public:

    CursorI() {}
    virtual ~CursorI() {}

    virtual OSCURSORHANDLE getOSHandle()=0;
  
  protected:

    RECVS_DISPATCH;
    
};

#ifdef WASABI_COMPILE_SKIN

class COMEXP SkinCursor : public CursorI, public SkinCallbackI {

  public:

    SkinCursor();
    SkinCursor(const char *elementid);
    virtual ~SkinCursor();
  
    virtual void setCursorElementId(const char *id);
    virtual int skincb_onReset();
    virtual OSCURSORHANDLE getOSHandle();
    virtual void reset();

  private:
    
    String name;
    OSCURSORHANDLE cursor;
};

#endif

#endif
