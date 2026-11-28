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
#ifndef __SKINMODREADER_H
#define __SKINMODREADER_H

#include "../studio/xmlreader.h"
#include "../bfc/string.h"

// -----------------------------------------------------------------------
class AddOnInfo {
  public:

    AddOnInfo() { part = -1; gotdata=0;}
    virtual ~AddOnInfo() {}

    const char *getFullName() { return fullname; }
    const char *getVersion() { return version; }
    const char *getComment() { return comment; }
    const char *getAuthor() { return author; }
    const char *getEmail() { return email; }
    const char *getHomepage() { return homepage; }
    int getSkinPart() { return part; }
    void setSkinPart(int i) { part = i; }

    int gotData() { return gotdata; }

    int gotFullName() { return !fullname.isempty(); }
    int gotVersion() { return !version.isempty(); }
    int gotComment() { return !comment.isempty(); }
    int gotAuthor() { return !author.isempty(); }
    int gotEmail() { return !email.isempty(); }
    int gotHomepage() { return !homepage.isempty(); }
    void setFilename(const char *s) { filename = s; }
    const char *getFilename() { return filename; }

protected:

  String fullname;
  String version;
  String comment;
  String author;
  String email;
  String homepage;
  String filename;
  int gotdata;
  int part;
};

// -----------------------------------------------------------------------
class AddOnReader : public AddOnInfo, public XmlReaderCallbackI {
public:
  AddOnReader(const char *filename);
  virtual void xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *s);
  virtual int xmlReaderDisplayErrors() { return 0; }
};

#endif