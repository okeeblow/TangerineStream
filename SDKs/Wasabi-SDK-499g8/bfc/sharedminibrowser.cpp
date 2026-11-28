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
#include "sharedminibrowser.h"
#include "wnds/skinwnd.h"
#include "../studio/api.h"
#include "../common/mainminibrowser.h"

void SharedMiniBrowser::navigateUrl(const char *url) {
  if (!m_monitor) {
    m_monitor = new SkinMonitor();
  }

  if (!MainMiniBrowser::getScriptObject()) {

    if (!m_inserted) {
      String xml = "buf:\n";
      xml += "<WinampAbstractionLayer>\n";
      xml += "  <groupdef id=\"addon.shared.minibrowser\" name=\"MiniBrowser\">\n";
      xml += "    <browser mainmb=\"1\" x=\"0\" y=\"0\" w=\"0\" h=\"0\" relatw=\"1\" relath=\"1\" />\n";
      xml += "  </groupdef>\n";
      xml += "</WinampAbstractionLayer>\n";
      WASABI_API_SKIN->loadSkinFile(xml);
      m_inserted = 1;
    }

    SkinWnd("addon.shared.minibrowser", WASABISTDCONTAINER_RESIZABLE_NOSTATUS);
    ASSERTPR(MainMiniBrowser::getScriptObject() != NULL, "Something is really wrong with wasabi");
  }

  MainMiniBrowser::navigateUrl(url);
  MainMiniBrowser::popMb();
}

void SharedMiniBrowser::shutdown() {
  if (m_monitor) delete m_monitor;
  m_monitor = NULL;
}

int SharedMiniBrowser::m_inserted = 0;
SkinMonitor *SharedMiniBrowser::m_monitor = NULL;