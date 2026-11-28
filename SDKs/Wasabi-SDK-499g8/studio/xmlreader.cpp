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
#include "xmlreader.h"
#include "xml/xmlparse.h"
#include "xml/xmltok.h"
#include "../bfc/util/pathparse.h"
#include "../common/wildcharsenum.h"
#include "../common/url.h"
#include "api.h"
#include "services/svc_xmlprov.h"
#include "varmgr.h"
#include "xmlparamsi.h"

void XmlReader::registerCallback(const char *matchstr, XmlReaderCallback *callback) {
  if (matchstr == NULL || callback == NULL) return;
  xmlreader_cb_struct *s = new xmlreader_cb_struct(matchstr, TYPE_CLASS_CALLBACK, callback);
  callback_list.addItem(s, 0);	// mig: add to FRONT of list... so we don't step on hierarchical toes.
}

void XmlReader::registerCallback(const char *matchstr, void (* callback)(int, const char *, const char *, XmlReaderParams *)) {
  if (matchstr == NULL || callback == NULL) return;
  xmlreader_cb_struct *s = new xmlreader_cb_struct(matchstr, TYPE_STATIC_CALLBACK, (XmlReaderCallback *)callback);
  callback_list.addItem(s, 0);	// mig: add to FRONT of list... so we don't step on hierarchical toes.
}

void XmlReader::unregisterCallback(void *callback) {
  for(int i=0;i<callback_list.getNumItems();i++) {
    if (callback_list[i]->callback==callback) {
      delete callback_list[i];
      callback_list.delByPos(i);
      i--;
    }
  }
}

int XmlReader::loadFile(const char *filename, const char *incpath, void *only_this_callback, const char **params, int isinclude) {
  if (!isinclude) currentpos = "";
  FILE *fp=NULL;
  svc_xmlProvider *svc = NULL;
  String fn;

  if (!STRNINCMP(filename, "buf:")) {
    return doLoadFile(NULL, NULL, filename, incpath, only_this_callback, params);
  } else {
    fn = filename;
    if (incpath) fn.prepend(incpath);

    WildcharsEnumerator e(fn);
  
    if (e.getNumFiles() > 0) { // we're including multiple files

      for (int i=0;i<e.getNumFiles();i++) {
        fp=fopen(e.enumFile(i),"rb");
        if (!fp) return 0;

        PathParser pp(e.enumFile(i));
        doLoadFile(fp, svc, pp.getLastString(), incpath, only_this_callback);
      }

      return 1;
    
    } else {

      fp=fopen(fn,"rb");
      if (!fp)   // file failed to open, try xmlproviders
        svc = XmlProviderEnum(filename).getNext();

      return doLoadFile(fp, svc, filename, incpath, only_this_callback, params);
    }
  }
  // some compilers are stupid!
  return 0;
}

const char *XmlReader::getIncludePath() {
  return include_stack.top()->getValue();
}

int XmlReader::startXmlLoad(const char *filename, XmlReaderCallback *only_this_object) {
  FILE *fp=FOPEN(filename,"rb");
  if(!fp) return 0;

  void *parser=XML_ParserCreate(0);
  XML_SetUserData(parser, only_this_object);
  XML_SetElementHandler(parser, parserCallbackOnStartElement, parserCallbackOnEndElement);
  XML_SetCharacterDataHandler(parser, parserCallbackOnCharacterData);

  XmlFileHandle *xfh=new XmlFileHandle();
  xfh->m_parser=parser;
  xfh->m_file=fp;
  xfh->m_filename=filename;
  xfh->m_callback=only_this_object;

  return (int)xfh;
}

int XmlReader::loadChunk(int handle, int chunksize) {
  XmlFileHandle *xfh=(XmlFileHandle *)handle;
  MemBlock<char> buf(chunksize);
  int readbytes=FREAD(buf,1,chunksize,xfh->m_file);
  int done=readbytes<chunksize;
  if (!parseBuffer(xfh->m_parser, buf, readbytes, done,xfh->m_callback,xfh->m_filename)) return 0;
  if(done) return 0;
  return 1;
}

void XmlReader::close(int handle) {
  XmlFileHandle *xfh=(XmlFileHandle *)handle;
  XML_ParserFree(xfh->m_parser);
  FCLOSE(xfh->m_file);
  delete(xfh);
}

void XmlReader::parserCallbackOnStartElement(void *userData, const char *name, const char **parms) {
  if (STRCASEEQL(name, "includeshit")) return;
  if (STRCASEEQL(name, "include")) {
#ifdef WASABI_COMPILE_WNDMGR
    const char *p = WASABI_API_WNDMGR->varmgr_translate(peekParam("file", parms));
#else
    const char *p = peekParam("file", parms);
#endif
    if (p) {
      if(userData) {
        // check for include files thing
        XmlReaderCallback *cb=(XmlReaderCallback *)userData;
        if(!cb->xmlReaderIncludeFiles()) return;
      }
      const char *file = Std::filename(p);
      int fnlen = STRLEN(file);
      String includepath = p;
      includepath.trunc(-fnlen);
 
      // if nonrelative path, use top of stack
      if (!Std::isRootPath(p))
        includepath.prepend(include_stack.top()->getValue());

      loadFile(file, includepath, userData, parms, 1);
      cont_char="";
      return;
    }
  }

  if (!currentpos.isempty())
    currentpos.cat("/");
  currentpos.cat(name);

  XmlReaderParamsI p;
  int i;
  for(i=0;parms[i];i+=2) { //CT> removed api-> for speed
    // BU String* for no exploit
#ifdef WASABI_COMPILE_WNDMGR
    String *str = PublicVarManager::translate_nocontext(parms[i+1]);
    Url::decode(*str);
    p.addItem(parms[i], str->getValue());
    delete str;
#else
    String ss = parms[i+1];
    Url::decode(ss);
    p.addItem(parms[i], ss);
#endif
  }

  foreach(callback_list)
    if(!userData || userData==callback_list.getfor()->callback) {
      if(Std::match(callback_list.getfor()->matchstr.v(), currentpos)) {
        switch(callback_list.getfor()->type) {
          case TYPE_CLASS_CALLBACK: 
            if(!cont_char.isempty()) callback_list.getfor()->callback->xmlReaderOnCharacterDataCallback(currentpos, name, cont_char);
            callback_list.getfor()->callback->xmlReaderOnStartElementCallback(currentpos, name, &p);
            break;
          case TYPE_STATIC_CALLBACK:
            ((void (*)(int, const char*, const char *, XmlReaderParams *))callback_list.getfor()->callback)(1, currentpos, name, &p);
            break;
        }
        if(userData) break;
      }
    }
  endfor

  cont_char="";
}

void XmlReader::parserCallbackOnEndElement(void *userData, const char *nameC) {
  if (STRCASEEQL(nameC, "includeshit")) return;
  if (STRCASEEQL(nameC, "include")) return;

  const char *s=currentpos.getValue();

  foreach(callback_list)
    if(!userData || userData==callback_list.getfor()->callback) {
      if(Std::match(callback_list.getfor()->matchstr.v(), s)) {
        switch(callback_list.getfor()->type) {
          case TYPE_CLASS_CALLBACK: 
            if(!cont_char.isempty()) callback_list.getfor()->callback->xmlReaderOnCharacterDataCallback(s, nameC, cont_char);
            callback_list.getfor()->callback->xmlReaderOnEndElementCallback(s, nameC);
            break;
          case TYPE_STATIC_CALLBACK:
            ((void (*)(int, const char *, const char *, XmlReaderParams *))callback_list.getfor()->callback)(0, s, nameC, NULL);
            break;
        }
        if(userData) break;
      }
    }
    endfor

  char *p=(char *)Std::scanstr_back(s, "/", 0);
  if(!p) currentpos="";
  else {
    *p=0;
  }

  cont_char="";
}

void XmlReader::parserCallbackOnCharacterData(void *userData, const char *str, int len) {
  cont_char.catn(str,len);
}

const char *XmlReader::peekParam(const char *txt, const char **parms) {
  for (int i=0;parms[i];i+=2)
    if (STRCASEEQL(parms[i],txt)) return parms[i+1];
  return NULL;
}

int XmlReader::doLoadFile(FILE *fp, svc_xmlProvider *svc, const char *filename, const char *incpath, void *only_this_callback, const char **params) {
  
  // Set us up for being able to handle buffers, too.
  int buf_pos = 0;
  const char *buffer = NULL;
  if (!fp && !svc) {
    if (!STRNINCMP(filename, "buf:")) {
      buffer = filename + STRLEN("buf:");
    } else {
      return 0;
    }
  }

  void *parser=XML_ParserCreate(0);
  XML_SetUserData(parser, only_this_callback);
  XML_SetElementHandler(parser, parserCallbackOnStartElement, parserCallbackOnEndElement);
  XML_SetCharacterDataHandler(parser, parserCallbackOnCharacterData);

  String *incr=new String(incpath);

  int includeshit=0;
  if(!include_stack.isempty()) {
    includeshit=1;
    XML_Parse(parser, "<includeshit>", 13, 0);
  }

  include_stack.push(incr);

  char buf[16384];
  const char *pbuf = buf;
  int done;
  do {
    unsigned long len;
    if (buffer) {
      int buflen = strlen(buffer + buf_pos); // remaining buffer.
      MEMCPY(buf, buffer + buf_pos, min(buflen,sizeof(buf)));
      if (buflen < sizeof(buf)) {
        len = buflen;
        done = 1;
      } else {
        len = sizeof(buf);
        buf_pos += sizeof(buf);
      }
    } else if (fp) {
      len=fread(buf,1,sizeof(buf),fp);
      done = len < sizeof(buf);
    } else {
      XmlReaderParamsI p;
      if(params)
        for(int i=0;params[i];i+=2) {
#ifdef WASABI_COMPILE_WNDMGR
          String ss;
          ss = params[i+1];
          ss = WASABI_API_WNDMGR->varmgr_translate(ss);
          Url::decode(ss);
          p.addItem(params[i], ss);

#else
          String ss = params[i+1];
          Url::decode(ss);
          p.addItem(params[i], ss);
#endif
      }
      pbuf = svc->getXmlData(filename, incpath, &p);
      if (pbuf == NULL) pbuf = "";
      len = STRLEN(pbuf);
      done = 1;
    }
    if (pbuf && len > 0)
      if(!parseBuffer(parser,pbuf,len,includeshit?0:done,(XmlReaderCallback *)only_this_callback,filename))
        break;
  } while (!done);

  if(includeshit) XML_Parse(parser, "</includeshit>", 14, 1);

  XML_ParserFree(parser);
  if (fp) fclose(fp);

  include_stack.pop(&incr);
  delete incr;

  if (svc) {
    SvcEnum::release(svc); 
    svc = NULL;
  }

  return 1;
}

int XmlReader::parseBuffer(void *parser, const char *xml, int size, int final, XmlReaderCallback *only_this_callback, const char *filename) {
  if (!XML_Parse(parser, xml, size, final)) {
    int disperr=1;
    if(only_this_callback) {
      foreach(callback_list)
        xmlreader_cb_struct *c=callback_list.getfor();
        if(c->callback==only_this_callback && c->type==TYPE_CLASS_CALLBACK){
          if(!c->callback->xmlReaderDisplayErrors()) disperr=0;
          break;
        }
      endfor
    }
    StringPrintf txt("error parsing xml layer (%s)\n",filename);
    StringPrintf err("%s at line %d\n",
      XML_ErrorString(XML_GetErrorCode(parser)),
      XML_GetCurrentLineNumber(parser));
    if (disperr) {
      Std::messageBox(err, txt, 0);
    } else {
      OutputDebugString(txt);
      OutputDebugString(err);
    }
    currentpos="";
    return 0;
  }
  return 1;
}

PtrList <xmlreader_cb_struct> XmlReader::callback_list;
String XmlReader::currentpos;
String XmlReader::cont_char;
Stack < String *> XmlReader::include_stack;
