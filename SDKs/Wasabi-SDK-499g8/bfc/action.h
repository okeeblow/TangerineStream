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
#ifndef __ACTION_H
#define __ACTION_H

#include "../studio/services/svc_action.h"

class Action {
	public:
		Action(const char *action, const char *param=NULL, int p1=0, int p2=0, void *data=NULL, int datalen=0, RootWnd *source=NULL);
    virtual ~Action();

  private:
    svc_action *action_svc;
};

#endif
