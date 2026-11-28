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

#include "bpl.h"

#include <studio/api.h>
#include <common/metatags.h>
// xml stuff
#include <bfc/xml/xmlwrite.h>
#include <studio/xmlreader.h>

// i/o
enum {
	ID_NONE,
//	ID_PLAYSTRING,
	ID_NAME,
	ID_ARTIST,
	ID_TITLE,
	ID_ALBUM,
	ID_GENRE,
	ID_YEAR,
	ID_COMMENT,
	ID_TRACK,
	ID_LENGTH,
	ID_LENGTHSTR
};
static char curPlayString[WA_MAX_PATH];
static char curName[WA_MAX_PATH];
static char curArtist[WA_MAX_PATH];
static char curTitle[WA_MAX_PATH];
static char curAlbum[WA_MAX_PATH];
static int curLen=0;
static int curID=ID_NONE;
static int curNumEntries;
static int curNumProcessedEntries;

int B4sLoader::readPlaylist(const char *filename) {
  setPlaylistPathFromFilename(filename);

  *curPlayString = 0;
  *curName = 0;
  *curArtist = 0;
  *curTitle = 0;
  *curAlbum = 0;
  curLen = 0;
  curNumProcessedEntries=0;
  WASABI_API_XML->xmlreader_registerCallback("WasabiXML/playlist/*", this);
  WASABI_API_XML->xmlreader_registerCallback("WasabiXML/playlist/*", this);
  WASABI_API_XML->xmlreader_registerCallback("WinampXML/playlist/*", this);//CUT someday
  WASABI_API_XML->xmlreader_registerCallback("WinampXML/playlist/*", this);//CUT someday
  WASABI_API_XML->xmlreader_loadFile(filename, this);
  WASABI_API_XML->xmlreader_unregisterCallback(this);
  return curNumProcessedEntries;
}

// xml parser handlers
void B4sLoader::xmlReaderOnStartElementCallback(const char *xmlpath, const char *xmltag, XmlReaderParams *params) {
  if (!STRICMP(xmltag,"playlist")) {
    curNumEntries=0;
    int n=params->getItemValueInt("num_entries",-1);
    if(n!=-1) api->startCompleted(curNumEntries=n);
    setName(params->getItemValue("label"));
  } else if (!STRICMP(xmltag,"entry")) {
    curPlayString[0]=0;
    curName[0]=0;
    curArtist[0]=0;
    curAlbum[0]=0;
    curLen = -1;
//    curID=ID_NONE;
    STRNCPY(curPlayString, params->getItemValue("playstring"), sizeof(curPlayString)-1);
//CUT    curRel = ATOI(params->getItemValue("relative"));
  }
//	if(!stricmp(name,MT_PLAYSTRING)) curID=ID_PLAYSTRING;
/*  if (!STRICMP(xmltag, MT_NAME)) curID=ID_NAME;
  if (!STRICMP(xmltag, MT_LENGTH)) curID=ID_LENGTH;*/
//  if (!STRICMP(name, MT_ARTIST)) curID=ID_ARTIST;
//  if (!STRICMP(name, MT_TITLE)) curID=ID_TITLE;
//  if (!STRICMP(name, MT_ALBUM)) curID=ID_ALBUM;
//  if (!STRICMP(name, MT_GENRE)) curID=ID_GENRE;
//  if (!STRICMP(name, MT_YEAR)) curID=ID_YEAR;
//  if (!STRICMP(name, MT_COMMENT)) curID=ID_COMMENT;
//  if (!STRICMP(name, MT_TRACK)) curID=ID_TRACK;
//  if (!STRICMP(name, MT_LENGTHSTR)) curID=ID_LENGTHSTR;
}

void B4sLoader::xmlReaderOnEndElementCallback(const char *xmlpath, const char *xmltag) {
  if (!STRICMP(xmltag,"entry")) {
    if(curPlayString[0]) {
      removeSlashes(curPlayString);
      xmlUnescape(curPlayString);
      addPlaylistEntry(curPlayString, curName);

//FUCKO: it just takes forever to load long playlists with this enabled
#if 1
if (/*!already_had_it &&*/ STRNCMP(curName, "file:", 5) != 0) {// if it's not a file
      if (*curArtist) {
        xmlUnescape(curArtist);
        applyMetaData(curPlayString, MT_ARTIST, curArtist, STRLEN(curArtist)+1, MDT_STRINGZ);
      }
      if (*curAlbum) {
        xmlUnescape(curAlbum);
        applyMetaData(curPlayString, MT_ALBUM, curAlbum, STRLEN(curAlbum)+1, MDT_STRINGZ);
      }
      if (*curTitle) {
        xmlUnescape(curTitle);
        applyMetaData(curPlayString, MT_TITLE, curTitle, STRLEN(curTitle)+1, MDT_STRINGZ);
      }
#if 1
      // set length if we got one
//      int len = api->metadb_getLength(item);
      if (curLen > 0/* && len < 0 && len != curLen*/) {
        applyMetaData(curPlayString, MT_LENGTH, (char *)&curLen, sizeof curLen, MDT_TIME);
      }
#endif
}
#endif
//CUT      if (curNumEntries != 0) api->incCompleted();
      curNumProcessedEntries++;
    }
#ifdef ASSERTS_ENABLED
if ((curNumProcessedEntries % 100) == 0) api->console_outputString(0, StringPrintf("loaded %d: %s", curNumProcessedEntries, curPlayString));
#endif

  }

//  curID=ID_NONE;
}

void B4sLoader::xmlReaderOnCharacterDataCallback(const char *xmlpath, const char *xmltag, const char *s) {
  if (!STRICMP(xmltag, MT_NAME)) {
    int l = MIN<int>((sizeof(curName) - 1) , STRLEN(s));
    STRNCPY(curName, s, l);
    curName[l] = 0;
  }
  if (!STRICMP(xmltag, MT_ARTIST)) {
    int l = MIN<int>((sizeof(curArtist) - 1) , STRLEN(s));
    STRNCPY(curArtist, s, l);
    curArtist[l] = 0;
  }
  if (!STRICMP(xmltag, MT_TITLE)) {
    int l = MIN<int>((sizeof(curTitle) - 1) , STRLEN(s));
    STRNCPY(curTitle, s, l);
    curTitle[l] = 0;
  }
  if (!STRICMP(xmltag, MT_ALBUM)) {
    int l = MIN<int>((sizeof(curAlbum) - 1) , STRLEN(s));
    STRNCPY(curAlbum, s, l);
    curAlbum[l] = 0;
  }
  if (!STRICMP(xmltag, MT_LENGTH)) curLen = ATOI(s);
}

B4sWriter::B4sWriter()
  : enable_metadata(FALSE), w(NULL) {
}

void B4sWriter::enableMetadata(int enabled) {
  enable_metadata = enabled;
}

int B4sWriter::beginWrite(const char *filename, int n, const char *label) {
  w = new XMLWrite(filename, "WasabiXML");
  w->comment(StringPrintf("Generated by: %s", WASABI_API_APP->main_getVersionString()));

  w->pushCategoryAttrib("playlist");
  w->writeCategoryAttrib("num_entries", n);
  if (label != NULL) w->writeCategoryAttrib("label", label);
  w->closeCategoryAttrib();

  char buf[WA_MAX_PATH];
  STRNCPY(buf, filename, WA_MAX_PATH);
  buf[WA_MAX_PATH-1] = 0;
  const char *p = Std::scanstr_back(buf, DIRCHARSTR, NULL);
  if (p == NULL) *buf = 0;
  else {
    const_cast<char *>(p)[1] = 0;
  }
  path = buf;

  return 1;
}

//m3u.cpp
int makeRelative(char *filename, const char *path);

void B4sWriter::writeEntry(const char *playstring) {
  ASSERT(w != NULL);
  ASSERT(playstring != NULL);
  w->pushCategoryAttrib("entry");
  char buf[WA_MAX_PATH];
  STRNCPY(buf, playstring, sizeof(buf));
  buf[WA_MAX_PATH-1] = 0;
  int rel = 0;
  // only file gets relative pathing
  if (!STRNINCMP(buf, "file:")) {
    STRCPY(buf, buf+STRLEN("file:"));
    rel = makeRelative(buf, path);
  }
  
  w->writeCategoryAttrib(MT_PLAYSTRING, buf);
  if (rel) w->writeCategoryAttrib("relative", "1");
  w->closeCategoryAttrib();

  if (enable_metadata) {
    xmlwrite(playstring, MT_NAME);
    xmlwrite(playstring, MT_ARTIST);
    xmlwrite(playstring, MT_TITLE);
    xmlwrite(playstring, MT_ALBUM);
    int len = api->metadb_getLength(playstring);
    if (len >= 0) w->writeAttrib(MT_LENGTH, api->metadb_getLength(playstring));
  }

  w->popCategory();
}

void B4sWriter::endWrite() {
  w->popCategory();	//playlist
  delete w; w = NULL;
}

void B4sWriter::xmlwrite(const char *playstring, const char *tag) {
  char buf[WA_MAX_PATH]="";
  int r = api->metadb_getMetaData(playstring, tag, buf, sizeof buf);
  if (r > 1) w->writeAttrib(tag, buf);
}
