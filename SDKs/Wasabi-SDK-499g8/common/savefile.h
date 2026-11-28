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
#ifndef _SAVEFILE_H
#define _SAVEFILE_H

#include "common.h"

#include "modal.h"
#include "../bfc/string.h"

class RootWnd;

class COMEXP SaveFileWnd : public Modal {
public:
  SaveFileWnd(const char *ident=NULL);
/**
  Sets the initial directory for the picker. If force is FALSE, this directory
  will only be used if no previous directory has been saved for the indent.
  If TRUE, it will always be used.
*/
  void setInitialDir(const char *dir, int force=FALSE);
  int getSaveFile(RootWnd *parent, const char *ext, const char *suggext);
  int getSaveFileW(RootWnd *parent, const char *ext, const char *suggext);
  const char *getFilename() { return filename; }

private:
  String identifier;
  String filename;
  String initial_dir;
  int force_initial_dir;
};

#endif
