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
#include "filereaderapi.h"
#include "../../studio/fileread.h"

api_fileReader *fileApi = NULL;


FileReaderApi::FileReaderApi() {
}

FileReaderApi::~FileReaderApi() {
}

void *FileReaderApi::fileOpen(const char *filename, const char *mode) {
  return FileReaders::open(filename, mode);
}

void FileReaderApi::fileClose(void *fileHandle) {
  FileReaders::close(fileHandle);
}

int FileReaderApi::fileRead(void *buffer, int size, void *fileHandle) {
  return FileReaders::read(buffer, size, fileHandle);
}

int FileReaderApi::fileWrite(const void *buffer, int size, void *fileHandle) {
  return FileReaders::write(buffer, size, fileHandle);
}

int FileReaderApi::fileSeek(int offset, int origin, void *fileHandle) {
  return FileReaders::seek(offset, origin, fileHandle);
}

int FileReaderApi::fileTell(void *fileHandle) {
  return FileReaders::tell(fileHandle);
}

int FileReaderApi::fileGetFileSize(void *fileHandle) {
  return FileReaders::getFileSize(fileHandle);
}

int FileReaderApi::fileExists(const char *filename) {
  return FileReaders::exists(filename);
}

int FileReaderApi::fileRemove(const char *filename) {
  return FileReaders::remove(filename);
}

int FileReaderApi::fileRemoveUndoable(const char *filename) {
  return FileReaders::removeUndoable(filename);
}

int FileReaderApi::fileMove(const char *filename, const char *destfilename) {
  return FileReaders::move(filename, destfilename);
}

