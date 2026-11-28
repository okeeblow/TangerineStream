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
#include "../../bfc/std.h"

#include "exampleoswnd.h"
#include "exampleoswndwnd.h"
#include "resource.h"

#include "../../bfc/canvas.h"
#include "../../common/xlatstr.h"

// I don't have any HWND code to play with off the top of my head.
HWND myExternalHWND = NULL;
int myExternalWidth = AUTOWH; 
int myExternalHeight = AUTOWH;
// So if you run this example as is, it won't do much.  Eh?

// This is more like a template project for people to use external HWNDs than an example.

//  Methods required by Window Creation Services
const char *ExampleOSWndWnd::getWindowTypeName() { return /**/"ExampleOSWnd Window"/*EDITME*/; }
GUID ExampleOSWndWnd::getWindowTypeGuid() { return the->getGUID(); }
void ExampleOSWndWnd::setIconBitmaps(ButtonWnd *button) {
  button->setBitmaps(the->gethInstance(), IDB_TAB_NORMAL, NULL, IDB_TAB_HILITED, IDB_TAB_SELECTED);
}

int ExampleOSWndWnd::onInit() {
  // Here is where you call your CreateWindow to do whatever create window stuff you need to do.
  // If your window is created by someone else, somewhere else, just bind the varable, here.
#ifdef WIN32
  RECT r = clientRect();
  // HOWEVER, in Win32, your window needs to have this window as a parent.
  // the gethWnd() and gethInstance() methods are provided to give Win32
  // specific code the ability to create child windows.
  myExternalHWND = CreateWindow("EDIT", NULL,
      WS_CHILD | ES_AUTOHSCROLL | ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN,
      r.left, r.top, r.right - r.left, r.bottom - r.top,
      gethWnd(), NULL,
      gethInstance(), NULL);
#endif
  // Remember to call your parent method.  Do this AFTER creating your HWND, since
  // the parent method will call getOSHandle();
  EXAMPLEOSWNDWND_PARENT::onInit();
  return 1;
}

ExampleOSWndWnd::~ExampleOSWndWnd() {
  // Here is where you call your DestroyWindow, or tell your external code you're done with the HWND.
#ifdef WIN32
  DestroyWindow(myExternalHWND);
  myExternalHWND = NULL;
#endif
}

int ExampleOSWndWnd::getPreferences(int what) {
  // Your HWND's display area will be clipped within the area of the component window.

  // getPreferences allows you to control what size the engine allows you to be.
  switch (what) {
    case SUGGESTED_W:
    case MINIMUM_W:
    case MAXIMUM_W:
      return myExternalWidth;
    case SUGGESTED_H:
    case MINIMUM_H:
    case MAXIMUM_H:
      return myExternalHeight;
  }
  // If you wanna be doubly tricky, you could actually use GetWindowLong calls on
  // your own HWND to figure out what you should return for your width and height
  // there, eh?  Runtime metrics are always better than hardcoded values, right?
  return EXAMPLEOSWNDWND_PARENT::getPreferences(what);
  // But that's not my code, that's YOUR code.
}


//
//  All you have to do is return your HWND here, the system will keep track of you.
HWND ExampleOSWndWnd::getOSHandle() {
  return myExternalHWND;
}
//  OSWnd handles mapping the hiding and showing, moving, and resizing of the window to the given HWND.