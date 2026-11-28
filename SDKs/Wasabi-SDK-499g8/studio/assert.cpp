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
#ifdef WIN32
#include <windows.h>
#endif

#include "assert.h"

#ifdef ASSERTS_ENABLED

static int in_assert = 0;

// we try to use wsprintf because it can make for a smaller .exe
#ifdef WIN32
#define ASSERT_SPRINTF wsprintf
#elif defined(LINUX)
#define ASSERT_SPRINTF sprintf
#else
#error port me!
#endif

#ifdef WIN32
static int isDebuggerPresent() {
  HINSTANCE kernel32 = LoadLibrary("kernel32.dll");
  if (kernel32 == NULL) return 0;
  BOOL (WINAPI *IsDebuggerPresent)() = NULL;
  IsDebuggerPresent = (BOOL (WINAPI *)())GetProcAddress(kernel32, "IsDebuggerPresent");
  if (IsDebuggerPresent == NULL) return 0;
  int ret = (*IsDebuggerPresent)();
  FreeLibrary(kernel32);
  return ret;
}
#endif

void _assert_handler(const char *reason, const char *file, int line) {
  char msg[4096];
  ASSERT_SPRINTF(msg, "Expression: %s\nFile: %s\nLine: %d\n", reason, file, line);
  OutputDebugString(msg);
#ifdef WIN32
  if (in_assert) __asm { int 3 };
  in_assert = 1;

  ATOM a = AddAtom("BYPASS_DEACTIVATE_MGR"); // so we don't need to call api->appdeactivation_setbypass

  if (isDebuggerPresent() || MessageBox(NULL, msg, "Assertion failed", MB_OKCANCEL|MB_TASKMODAL) == IDCANCEL) {
    __asm { int 0x3 };
  } else
    ExitProcess(0);
  DeleteAtom(a);
#elif defined(LINUX)
  kill(getpid(), SIGSEGV );
#else
#error port me!
#endif
  in_assert = 0;
}

void _assert_handler_str(const char *string, const char *reason, const char *file, int line) {
  char msg[4096];
  ASSERT_SPRINTF(msg, "\"%s\"\nExpression: %s\nFile: %s\nLine: %d\n", string, reason ? reason : "", file, line);
  OutputDebugString(msg);

#ifdef WIN32
  if (in_assert) __asm { int 3 };
  in_assert = 1;

  ATOM a = AddAtom("BYPASS_DEACTIVATE_MGR");
  if (isDebuggerPresent() || MessageBox(NULL, msg, "Assertion failed", MB_OKCANCEL|MB_TASKMODAL) == IDCANCEL) {
    __asm { int 0x3 };
  } else
    ExitProcess(0);
  DeleteAtom(a);
#elif defined(LINUX)
  kill(getpid(), SIGSEGV );
#else
#error port me!
#endif
  in_assert = 0;
}

#endif
