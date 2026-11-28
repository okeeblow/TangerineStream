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
#include "../../bfc/api/api_filereader.h"

class FileReaderApi : public api_fileReaderI {
  public:
    FileReaderApi();
    virtual ~FileReaderApi();
    
    virtual void *fileOpen(const char *filename, const char *mode);
    virtual void fileClose(void *fileHandle);
    virtual int fileRead(void *buffer, int size, void *fileHandle);
    virtual int fileWrite(const void *buffer, int size, void *fileHandle);
    virtual int fileSeek(int offset, int origin, void *fileHandle);
    virtual int fileTell(void *fileHandle);
    virtual int fileGetFileSize(void *fileHandle);
    virtual int fileExists(const char *filename);
    virtual int fileRemove(const char *filename);
    virtual int fileRemoveUndoable(const char *filename);
    virtual int fileMove(const char *filename, const char *destfilename);
};
