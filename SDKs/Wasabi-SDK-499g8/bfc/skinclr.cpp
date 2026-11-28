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

#include <bfc/api/api_skin.h>

#include "skinclr.h"

#include <studio/api.h>

int *SkinColor::skin_iterator=0;

SkinColor::SkinColor(const char *_name, const char *colorgroup) : FilteredColor(0, (colorgroup == NULL || !*colorgroup) ? "Text" : colorgroup) {
  latest_iteration = -1;
//CUT  skin_iterator = NULL;
  setElementName(_name);
  ovr_grp = colorgroup;
  dooverride = 0;
  color_override = 0; 
}

ARGB32 SkinColor::v() {
  if (name.isempty()) return 0xffff00ff;

  if (!iteratorValid()) {
    val = NULL;	// pointer now invalid, must re-get
    latest_iteration = *skin_iterator;	// and then we'll be current
  // new pointer please

    const char *grp=NULL;
    COLORREF r;
    if (dooverride)
      r = color_override;
    else
      r = WASABI_API_SKIN->skin_getColorElement((char *)name.v(), &grp);
    if (ovr_grp == NULL && grp != NULL) setColorGroup(grp);
    FilteredColor::setColor(r);
    val = getColorRef();
  }
  if (val == NULL) return 0xFFFF00FF;

  return *val;
}

void SkinColor::setElementName(const char *_name) {
  name = _name;
  val = NULL;
  latest_iteration = 0;
}

void SkinColor::setColor(COLORREF c) {
  dooverride = 1; 
  color_override = c; 
  FilteredColor::setColor(color_override); 
}

int SkinColor::iteratorValid() {
  // fetch iterator pointer if necessary
  if (skin_iterator == NULL) {
    skin_iterator = WASABI_API_SKIN->skin_getIterator();
    ASSERT(skin_iterator != NULL);
  }

  // see if we're current
  return (*skin_iterator == latest_iteration);
}
