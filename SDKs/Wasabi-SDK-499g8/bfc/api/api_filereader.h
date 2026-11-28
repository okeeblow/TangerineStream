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
#ifndef __API_FILEREADER_H
#define __API_FILEREADER_H

#include "../dispatch.h"

class api_fileReader : public Dispatchable {
  public:
    void *fileOpen(const char *filename, const char *mode);
    void fileClose(void *fileHandle);
    int fileRead(void *buffer, int size, void *fileHandle);
    int fileWrite(const void *buffer, int size, void *fileHandle);
    int fileSeek(int offset, int origin, void *fileHandle);
    int fileTell(void *fileHandle);
    int fileGetFileSize(void *fileHandle);
    int fileExists(const char *filename);
    int fileRemove(const char *filename);
    int fileRemoveUndoable(const char *filename);
    int fileMove(const char *filename, const char *destfilename);

  enum {
    API_FILEREADER_FILEOPEN = 0,
    API_FILEREADER_FILECLOSE = 10,
    API_FILEREADER_FILEREAD = 20,
    API_FILEREADER_FILEWRITE = 30,
    API_FILEREADER_FILESEEK = 40,
    API_FILEREADER_FILETELL = 50,
    API_FILEREADER_FILEGETFILESIZE = 60,
    API_FILEREADER_FILEEXISTS = 70,
    API_FILEREADER_FILEREMOVE = 80,
    API_FILEREADER_FILEREMOVEUNDOABLE = 90,
    API_FILEREADER_FILEMOVE = 100,
  };
};

inline void *api_fileReader::fileOpen(const char *filename, const char *mode) {
  return _call(API_FILEREADER_FILEOPEN, (void *)NULL, filename, mode);
}

inline void api_fileReader::fileClose(void *fileHandle) {
  _voidcall(API_FILEREADER_FILECLOSE, fileHandle);
}

inline int api_fileReader::fileRead(void *buffer, int size, void *fileHandle) {
  return _call(API_FILEREADER_FILEREAD, (int)0, buffer, size, fileHandle);
}

inline int api_fileReader::fileWrite(const void *buffer, int size, void *fileHandle) {
  return _call(API_FILEREADER_FILEWRITE, (int)0, buffer, size, fileHandle);
}

inline int api_fileReader::fileSeek(int offset, int origin, void *fileHandle) {
  return _call(API_FILEREADER_FILESEEK, (int)0, offset, origin, fileHandle);
}

inline int api_fileReader::fileTell(void *fileHandle) {
  return _call(API_FILEREADER_FILETELL, (int)0, fileHandle);
}

inline int api_fileReader::fileGetFileSize(void *fileHandle) {
  return _call(API_FILEREADER_FILEGETFILESIZE, (int)0, fileHandle);
}

inline int api_fileReader::fileExists(const char *filename) {
  return _call(API_FILEREADER_FILEEXISTS, (int)0, filename);
}

inline int api_fileReader::fileRemove(const char *filename) {
  return _call(API_FILEREADER_FILEREMOVE, (int)0, filename);
}

inline int api_fileReader::fileRemoveUndoable(const char *filename) {
  return _call(API_FILEREADER_FILEREMOVEUNDOABLE, (int)0, filename);
}

inline int api_fileReader::fileMove(const char *filename, const char *destfilename) {
  return _call(API_FILEREADER_FILEMOVE, (int)0, filename, destfilename);
}

class api_fileReaderI : public api_fileReader {
  public:
    virtual void *fileOpen(const char *filename, const char *mode)=0;
    virtual void fileClose(void *fileHandle)=0;
    virtual int fileRead(void *buffer, int size, void *fileHandle)=0;
    virtual int fileWrite(const void *buffer, int size, void *fileHandle)=0;
    virtual int fileSeek(int offset, int origin, void *fileHandle)=0;
    virtual int fileTell(void *fileHandle)=0;
    virtual int fileGetFileSize(void *fileHandle)=0;
    virtual int fileExists(const char *filename)=0;
    virtual int fileRemove(const char *filename)=0;
    virtual int fileRemoveUndoable(const char *filename)=0;
    virtual int fileMove(const char *filename, const char *destfilename)=0;

  protected:
    RECVS_DISPATCH;
};

// {E357E736-4967-4279-B948-5073A186F565}
static const GUID fileReaderApiServiceGuid = 
{ 0xe357e736, 0x4967, 0x4279, { 0xb9, 0x48, 0x50, 0x73, 0xa1, 0x86, 0xf5, 0x65 } };

extern api_fileReader *fileApi;

#endif
