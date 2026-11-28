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

#include "playersvc.h"

#include "../studio/api.h"
#include "../studio/services/svc_droptarget.h"

#include "../common/filename.h"
#include "../common/fakedrag.h"
#include "../common/selectfile.h"

void PlayerSvc::openFile(const char *filename, FOURCC droptarg) {
  FilenameNC f(filename);
  FakeDragWnd fdw;
  fdw.addDragItem(DD_FILENAME, static_cast<Filename*>(&f));

  DropTargetEnum::throwDrop(droptarg, &fdw);
}

void PlayerSvc::openFiles(RootWnd *parent, const char *type, FOURCC droptarg) {
  const char *ext=api->core_getExtSupportedExtensions();

  SelectFile sf(parent ? parent : api->main_getRootWnd(), "Load ", "...");
  sf.runSelector(type, TRUE, ext);

  // copy filenames
  PtrList<FilenameNC> keep;
  FakeDragWnd fdw;
  for (int i = 0; i < sf.getNumFiles(); i++)
    fdw.addDragItem(DD_FILENAME, static_cast<Filename*>(keep.addItem(new FilenameNC(sf.enumFilename(i)))));

  DropTargetEnum::throwDrop(droptarg, &fdw);

  // delete copies of filenames
  keep.deleteAll();
}
