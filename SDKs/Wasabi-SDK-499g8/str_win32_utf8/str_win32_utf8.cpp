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
#include "../bfc/std.h"  // Always start with std.h
#include "../bfc/svc_enum.h"
#include "../bfc/encodedstr.h"
#include "../common/selectfile.h"
#include "../bfc/attribs/attrbool.h"

#include "str_win32_utf8.h"
#include "svc_str_win32_utf8.h"

#include <mbctype.h>

static WACNAME wac;
WACPARENT *the = &wac;                     

// Set this to 1 to compile and test the service implementation
// in the example code in the onCreate method.
#define EXAMPLE_CODE 0

// ===========================================================================
//
//  *** You MUST use a unique GUID for your WAC files.
//
// {BAA38E16-EE93-48dc-BD0A-55BC4EF44CAD}
GUID str_icu_utf8_guid = 
{ 0xbaa38e16, 0xee93, 0x48dc, { 0xbd, 0xa, 0x55, 0xbc, 0x4e, 0xf4, 0x4c, 0xad } };
//EDITME (hint: use guidgen.exe)

_bool enabled("Enable UTF8 Internationalization (requires restart)",TRUE);

WACNAME::WACNAME() : WACPARENT("UTF8 Internationalization") {

  registerAttribute(&enabled, NULL);
  registerSkinFile("xml/str_win32_utf8/str_win32_utf8.xml");
}

WACNAME::~WACNAME() {
}

// ===========================================================================
//
GUID WACNAME::getGUID() {
  return str_icu_utf8_guid;
}

void WACNAME::onRegisterServices() {
  // Register services MANUALLY, here.  We can now test our attrib because it was activated!
  if (enabled) {
    api->service_register( new waServiceFactoryT<svc_stringConverter, svc_str_win32_utf8> );
  }
}

void WACNAME::onCreate() {

 api->preferences_registerGroup("Internationalization.Preferences", "UTF8 Internationalization", getGUID());


// Even though you register your services in the constructor,
// the services are not available for use until you are called
// with your onCreate method.

#if EXAMPLE_CODE
  // *** Wanna USE a service?  Try this!

  // 1) Create an instance of the enumerator for the service you want.
  //    Often, this requires you to specify extra information
  //    as a construction parameter for the actual service type you
  //    wish to use.
  //    
  //    For the string conversion service, the construction parameter
  //    is the encoding type of the string you wish to translate to
  //    or from UTF-8.  So, for this example, we want to convert from
  //    UTF-16, so SvcStrCnv::UTF16 is the construction parameter.
  StringConverterEnum myServiceEnum(SvcStrCnv::UTF16);
  //    See "../common/svc_enum.h" for all the different service
  //    enumerators and their required construction params.


  // 2) Once your enumeration object is properly instantiated,
  //    you can enumerate (gasp!) through all of the "valid" service 
  //    instances currently registered with the system.
  //
  //    The getFirst() and getNext() methods for service enumerators will
  //    only return pointers to the service instances if they pass the
  //    internal testing of the enumerator.  In this example, we will be
  //    collecting all the different string conversion service instances
  //    who respond to SvcStrCnv::UTF16
  svc_stringConverter *myConv = myServiceEnum.getFirst();


  // 3) If you get one, there might be more.  On the other hand, you probably
  //    should not care which specific service instance you're talking to, so
  //    long as it gets your work done.  
  //
  //    For this example, just to show you the API, we'll list all registered
  //    service instances that can handle SvcStrCnv::UTF16 -- but the expectation is
  //    that you're only going to see one. :)  Unless you edit this project
  //    and make another one yourself (hint, hint).
  if (myConv != NULL) {
    svc_stringConverter *temp = myConv;
    OutputDebugString("Listing UTF16 compatible string conversion services:\n\n");
    do {
      OutputDebugString(StringPrintf("\t%08X\n\n",myConv));
      myConv = myServiceEnum.getNext();
    } while (myConv != NULL);
    myConv = temp; // go back to the first one for usage.  why?  why not?


  // 4) In any event, once you have a pointer to your basic service type,
  //    you may call the virtual methods on that pointer with impunity.
  //
  //    Our string converter is supposed to be converting strings, so let's
  //    feed it some example strings and see what it does with them, mmmmkay?

    // So here's a silly way to make a traditional, if unimpressive, utf16 string.
    short u16str[] = { 'h', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', 0 };
    // Please note that L"hello, world" isn't guaranteed to work on all compilers.
  
    // Then we convert it.
    int u8size = myConv->preflightToUTF8(SvcStrCnv::UTF16, u16str, sizeof(u16str));
    char *u8str = reinterpret_cast<char *>(api->sysMalloc(u8size));
    int u8out = myConv->convertToUTF8(SvcStrCnv::UTF16, u16str, sizeof(u16str), u8str, u8size);

    // And, to prove we did something vaguely interesting, we'll print it to the debug console
    OutputDebugString(StringPrintf("my u16str reads `%s`\n\n",u16str));//(yes, this should only print 'h' on little endian machines!)
    OutputDebugString(StringPrintf("my u8str (size = %d)\n reads `%s`\n\n", u8out, u8str));

    // Next, we'll throw a big buffer of random 16bit values at the unicode transcoder
    // to stress test it.  Cause we're naughty like that and we like to make our code cry.
    const int hugeSize = 8;
    short i, huge16str[hugeSize]; // autovariable arrays are dangerous!
    for (i = 0; i < hugeSize-1; i++) {
      huge16str[i] = (Std::random() & 0x7FFF) + 0xFF;
    }
    huge16str[hugeSize-1] = 0;

    // REMEMBER!  The conversion API wants the size in bytes, not the number of chars.
    int huge8size = myConv->preflightToUTF8(SvcStrCnv::UTF16, huge16str, sizeof(huge16str));
    char *huge8str = reinterpret_cast<char *>(api->sysMalloc(huge8size));
    int huge8out = myConv->convertToUTF8(SvcStrCnv::UTF16, huge16str, sizeof(huge16str), huge8str, huge8size);


    // After we convert that mess to utf8, we can convert it back again and then
    // compare the results.  NOTE: Only unicode-to-unicode transcodings are guaranteed
    // to be symmetrical like this!
    int void16size = myConv->preflightFromUTF8(SvcStrCnv::UTF16, huge8str, huge8out);
    char *void16str = reinterpret_cast<char *>(api->sysMalloc(void16size));
    int void16out = myConv->convertFromUTF8(SvcStrCnv::UTF16, huge8str, huge8out, void16str, void16size);
    short *new16str = reinterpret_cast<short *>(void16str);

    OutputDebugString(StringPrintf("my huge16str (size = %d)\n reads `%s`\n\n", sizeof(huge16str), huge16str));
    OutputDebugString(StringPrintf("my new16str (size = %d)\n reads `%s`\n\n", void16out, new16str));
    OutputDebugString(StringPrintf("my huge8str (size = %d)\n reads `%s`\n\n", huge8out, huge8str));

    // "newSizeInBytes" is filled by the function with the ALLOCATED SIZE OF
    // THE BUFFER RETURNED.  Whether or not the returned string uses the entirety
    // of the space is up to the implementor of the service and defined by the
    // way the character encoding defines the "end of a string"
    for (i = 0; (i < hugeSize) && (huge16str[i]) && (new16str[i]); i++ ) {
      if (huge16str[i] != new16str[i]) {
        OutputDebugString("Hey!  I suck!  They're not equal!\n\n");
        break;
      }
    }

    // Next set of tests, we want to test the SvcStrCnv::OSNATIVE conversion paths.
    // So let's generate some random 8bit numbers and see if we can convert
    // back and forth with SvcStrCnv::OSNATIVE like we can with "UTF16"
    char hugeOSstr[hugeSize]; // autovariable arrays are dangerous!
    for (i = 0; i < hugeSize-1; i++) {
      int pickAByte = Std::random() & 0xFF;
      if ((pickAByte < 0x80) && (pickAByte >= 0x20)) {
        hugeOSstr[i] = static_cast<char>(pickAByte);
      } else {
        i--;
      }
    }
    hugeOSstr[hugeSize-1] = 0;

    // Convert it to UTF8
    int OS8size = myConv->preflightToUTF8(SvcStrCnv::OSNATIVE, hugeOSstr, sizeof(hugeOSstr));
    char *OS8str = reinterpret_cast<char *>(api->sysMalloc(OS8size));
    int OS8out = myConv->convertToUTF8(SvcStrCnv::OSNATIVE, hugeOSstr, sizeof(hugeOSstr), OS8str, OS8size);
    // And back to OSNATIVE
    int voidOSsize = myConv->preflightFromUTF8(SvcStrCnv::OSNATIVE, OS8str, OS8out);
    char *voidOSstr = reinterpret_cast<char *>(api->sysMalloc(voidOSsize));
    int voidOSout = myConv->convertFromUTF8(SvcStrCnv::OSNATIVE, OS8str, OS8out, voidOSstr, voidOSsize);
    // And shame on me for not testing the return codes, but this is test code that I'm stepping through the debugger.
    // When you go to do your code, you should test for error conditions, because there's alot of them.
    // _I'M_ watching the return codes in the debugger and then shutting this code off.  You can't say the same.  :)
    OutputDebugString(StringPrintf("my hugeOSstr (size = %d)\n reads `%s`\n\n", sizeof(hugeOSstr), hugeOSstr));
    OutputDebugString(StringPrintf("my OS8str (size = %d)\n reads `%s`\n\n", OS8size, OS8str));
    OutputDebugString(StringPrintf("my voidOSstr (size = %d)\n reads `%s`\n\n", voidOSsize, voidOSstr));
    for (i = 0; (i < hugeSize) && (hugeOSstr[i]) && (voidOSstr[i]); i++ ) {
      if (hugeOSstr[i] != voidOSstr[i]) {
        OutputDebugString("Hey!  I suck!  They're not equal!\n\n");
        break;
      }
    }

    // Lastly, after we get done with using the converter, we MUST free the
    // returned memory buffers.  (Well, unless we _want_ to leak them, eh?)
    api->sysFree(u8str);
    api->sysFree(huge8str);
    api->sysFree(void16str);
    api->sysFree(OS8str);
    api->sysFree(voidOSstr);
    // And release the service instance.
    myServiceEnum.release(myConv);



  // =======================================================================
  // The one final point to be made here is that this UTF stuff will be simplified
  // by the use of the encodedstr object.  This EXAMPLE_CODE snippet was
  // originally written to simply test my service implementation for bugs and
  // was commented and expanded for clarity.  The important 1, 2, 3, 4 steps are
  // to show coders how to enumerate ANY service instances.  The rest of this
  // example snip should NOT be used as "the proper way to transcode strings"
  // unless you really alot hate the encodedstr object and must do it all yourself.

  // So, here's the paradigm for "EncodedStr" --

  // String is the object which holds UTF8 strings.
  // EncodedStr is the object which holds "Any Other" string -- specified by FOURCC encodingType

  // EncodedStr knows how to translate itself from and to a String object.

  // To printf everything:
#define xOutString(xStr)    OutputDebugString(StringPrintf("my " #xStr " (size = %d)\n reads `%s`\n\n", xStr.len() + 1, xStr))
#define xOutEncStr(xStr)    OutputDebugString(StringPrintf("my " #xStr " (size = %d)\n reads `%s`\n\n", xStr.getBufferSize(), xStr.getEncodedBuffer()))

    // Let's show it off.  First, let's take our random OSNATIVE string
    // and we stuff that osnative buffer into the encoded str.
    EncodedStr osnativeenc(SvcStrCnv::OSNATIVE, hugeOSstr, 
      sizeof(hugeOSstr), 0/*DON'T DELETE!!!!*/); 
    xOutEncStr(osnativeenc);
    // The object will delete your ram unless you say NO!
    // Since hugeOSStr is a stack variable, what do you think we HAVE to say?

    // The rest of these objects, however, handle their own memory.  Isn't that nice?

    // Now we can get from the "whatever" EncodedStr object a UTF8 String object
    String utf8enc;
    osnativeenc.convertToUTF8(utf8enc);
    xOutString(utf8enc);

    // And then make a new EncodedStr object from the utf8 object
    EncodedStr osnative8;
    osnative8.convertFromUTF8(SvcStrCnv::OSNATIVE, utf8enc);
    xOutEncStr(osnative8);

    // And back through String to be able to do a compare.
    String utf8reenc;
    osnativeenc.convertToUTF8(utf8reenc);
    xOutString(utf8reenc);

    // And just to show that this is easier, let's go back through utf16, too
    EncodedStr utf16(SvcStrCnv::UTF16, huge16str, 
      sizeof(huge16str), 0/*DON'T DELETE!!!!*/); 
    xOutEncStr(utf16);

    String utf8e16;
    utf16.convertToUTF8(utf8e16);
    xOutString(utf8e16);

    EncodedStr utf16re;
    utf16re.convertFromUTF8(SvcStrCnv::UTF16, utf8e16);
    xOutEncStr(utf16re);

    String utf8e16re;
    utf16re.convertToUTF8(utf8e16re);
    xOutString(utf8e16re);


    // Make a bold statement!
    ASSERT(utf8enc == utf8reenc);



    //
    //  Okay, now let's look at how we use files in the system.
    //
    SelectFile sf(api->main_getRootWnd(), "Load ", "...");
    sf.runSelector("files", TRUE, "*.*");

    int retval = _setmbcp(CP_UTF8);
    if (retval == 0) {
      OutputDebugString("Set ANSILIB codepage to UTF8.  Go me!\n");
    } else {
      OutputDebugString("Cannot set ANSILIB codepage to UTF8.  Damn me!\n");
    }

    for (i = 0; i < sf.getNumFiles(); i++) {
      OutputDebugString(StringPrintf("%s\n",sf.enumFilename(i)));
      FILE *my8File = fopen(sf.enumFilename(i),"rb");
      if (my8File != NULL) {
        OutputDebugString("Opened Properly.\n");
        fclose(my8File);
      } else {
        OutputDebugString("Scroooda Poooch.\n");
      }
    } 
  }
#endif//EXAMPLE_CODE
}

