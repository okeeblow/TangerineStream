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
#ifndef _SVCENUM_H
#define _SVCENUM_H


/*<?<autoheader/>*/
class waServiceFactory;
/*?>*/

// abstract base class
class NOVTABLE COMEXP SvcEnum {
protected:
  SvcEnum();

  void *_getNext(int global_lock = TRUE);
  void reset();

  virtual int _testService(void *)=0;

public:
#ifdef ASSERTS_ENABLED
  static int release(waServiceFactory *ptr) { ASSERTALWAYS("never ever call release() with a waServiceFactory * !!!"); return 0; }
#endif
  static int release(void *ptr);

  waServiceFactory *getLastFactory();

protected:
  FOURCC type;

private:
  int pos;
  waServiceFactory * factory;
};


#endif // _SVCENUM_H

