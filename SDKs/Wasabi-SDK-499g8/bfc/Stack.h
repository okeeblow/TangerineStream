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
//PORTABLE
#ifndef _STACK_H
#define _STACK_H

#include "../common/common.h"

#include "std.h"

// a self-growing stack. note that it never shrinks (for now)

class COMEXP StackBase {
protected:
  StackBase();
  ~StackBase();

  int push(void *item, int sizeofT, int increment);
  int peek();
  int peekAt(void *ptr, int n, int sizeofT);
  int getRef(void **ptr, int n, int sizeofT);
  void *top(int sizeofT);
  int pop(void *ptr, int sizeofT);
  int isempty();
  void purge();

private:
  int nslots, cur;
  char *stack;
};

template<class T>
class Stack : public StackBase {
public:
  int push(T item, int increment=-1) { return StackBase::push(&item, sizeof(T), increment); }
  using StackBase::peek;
  T top() { return *static_cast<T*>(StackBase::top(sizeof(T))); }
  int peekAt(T *ptr = NULL, int n = 0) { return StackBase::peekAt(ptr, n, sizeof(T)); }
  int getRef(T **ptr = NULL, int n = 0) { if (!ptr) return peek(); return StackBase::getRef((void **)&(*ptr), n, sizeof(T)); }
  int pop(T *ptr = NULL) { return StackBase::pop(ptr, sizeof(T)); }
  using StackBase::isempty;
  using StackBase::purge;
};

#endif
