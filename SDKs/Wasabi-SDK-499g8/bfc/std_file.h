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
#ifndef _STD_FILE_H
#define _STD_FILE_H

#include "std.h"

#ifndef _NOSTUDIO
// EXTC is used here as some .c files will use these functions
EXTC COMEXP FILE * FOPEN(const char *filename,const char *mode);
#ifdef LINUX
EXTC COMEXP FILE *FOPEN_NOCASE(const char *filename,const char *mode);
#endif
EXTC COMEXP int FCLOSE(FILE *stream);
EXTC COMEXP int FSEEK(FILE *stream, long offset, int origin);
EXTC COMEXP long FTELL(FILE *stream);
EXTC COMEXP size_t FREAD(void *buffer, size_t size, size_t count, FILE *stream);
EXTC COMEXP size_t FWRITE(const void *buffer, size_t size, size_t count, FILE *stream);
EXTC COMEXP char *FGETS( char *string, int n, FILE *stream );
EXTC COMEXP int FPRINTF( FILE *stream, const char *format , ...);
EXTC COMEXP int FGETSIZE(FILE *stream);
EXTC COMEXP const char *TMPNAM(char *string);
EXTC COMEXP const char *TMPNAM2(char *string, int val);
EXTC COMEXP int FEXISTS(const char *filename); // return 1 if true, 0 if not, -1 if unknown
EXTC COMEXP int UNLINK(const char *filename); // return 1 on success, 0 on error
EXTC COMEXP int ACCESS(const char *filename, int mode); // returns 0 if mode is set
#ifdef __cplusplus
// returns 1 on success, 0 on error, -1 if undoable deletes aren't supported
EXTC COMEXP int FDELETE(const char *filename, int permanently=TRUE);
#else
EXTC COMEXP int FDELETE(const char *filename, int permanently);
#endif

// 1 on success, 0 on fail
// can't move directories between volumes on win32
EXTC COMEXP int MOVEFILE(const char *filename, const char *destfilename);
// 1 on success, 0 on fail
EXTC COMEXP int COPYFILE(const char *filename, const char *destfilename);

#ifdef __cplusplus
namespace StdFile {
#endif
  int resolveShortcut(const char *filename, char *destfilename);
#ifdef __cplusplus
};
#endif

#ifdef WASABI_COMPILE_FILEREADER

#ifndef REAL_STDIO
#define fopen FOPEN
#define fclose FCLOSE
#define fseek FSEEK
#define ftell FTELL
#define fread FREAD
#define fwrite FWRITE
#define fgets FGETS
#define fprintf FPRINTF
#define unlink UNLINK
#define access ACCESS
#endif //real_stdio

#endif //WASABI_COMPILE_FILEREADER
#endif //_nostudio

#endif
