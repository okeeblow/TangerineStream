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
#include "shoutcast.h"
#include "shoutcastwnd.h"
#include "server.h"

#include "../common/metatags.h"

int Shoutcast::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch) {
  for(int i=0;i<g_servers.getNumItems();i++)
    g_servers[i]->processData(infos,chunk_list,killswitch);
  return 1;
}

int Shoutcast::corecb_onTitleChange(const char *title) {
  const char *cur=api->core_getCurrent(0);
  char txt[512];
  if (!(cur && (api->metadb_getMetaData(cur, MT_NAME, txt, 511, MDT_STRINGZ)))) {
    STRCPY(txt, title);
    char *p = txt+STRLEN(txt);
    while (p >= txt) {
      if (*p == '.') {
        *p = 0;
         break;
      }
    p--;
    }
  }

  for(int i=0;i<g_servers.getNumItems();i++)
    g_servers[i]->setSongTitle(txt);
  return 0;
}
