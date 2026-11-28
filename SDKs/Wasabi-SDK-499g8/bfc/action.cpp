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
#include "action.h"

Action::Action(const char *action, const char *param/* =NULL */, int p1/* =0 */, int p2/* =0 */, void *data/* =NULL */, int datalen/* =0 */, RootWnd *source/* =NULL */) {
  ActionEnum action_enum(action);
  action_svc = action_enum.getFirst();
  if (action_svc == NULL) return;
  action_svc->onAction(action, param, p1, p2, data, datalen, source);
}

Action::~Action() {
  if (action_svc != NULL)
    WASABI_API_SVC->service_release(action_svc);
}
