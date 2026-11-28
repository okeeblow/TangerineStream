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
#ifndef __API_SYSCBI_IMPL_H
#define __API_SYSCBI_IMPL_H

/*<?<autoheader/>*/
#include "api_syscb.h"
#include "api_syscbx.h"

class SysCallback;
/*?>*/

class api_syscbI : public api_syscbX {
public:
  DISPATCH(20) int syscb_registerCallback(SysCallback *cb, void *param = NULL);
  DISPATCH(10) int syscb_deregisterCallback(SysCallback *cb);
  DISPATCH(30) int syscb_issueCallback(int eventtype, int msg, int p1=0, int p2=0);
};

/*[interface.footer.h]
// -- generated code - edit in api_syscbi.h

// {57B7A1B6-700E-44ff-9CB0-70B92BAF3959}
static const GUID syscbApiServiceGuid = 
{ 0x57b7a1b6, 0x700e, 0x44ff, { 0x9c, 0xb0, 0x70, 0xb9, 0x2b, 0xaf, 0x39, 0x59 } };

extern api_syscb *sysCallbackApi;
*/

#endif // __API_SYSCBI_IMPL_H
