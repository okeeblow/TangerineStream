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
#include "wa2xmlprovider.h"
#include "../common/bitmap.h"

int Wa2XmlProvider::testDesc(const char *desc) {
  if (!STRCMP(desc, "svc:wa2skinxml"))
    return 1;
  if (!STRCMP(desc, "svc:wa2skinxml_player"))
    return 1;
  return 0;
}
                      
const char *Wa2XmlProvider::getXmlData(const char *desc, const char *incpath, XmlReaderParams *params) {
  if (!STRCMP(desc, "svc:wa2skinxml")) {
    static const char xml[]="<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      "<WinampAbstractionLayer version=\"0.6\">"
        "<include file=\"../default/xml/studio-elements.xml\"/>"
        "<include file=\"svc:wa2skinxml_player\"/>"
        "<include file=\"../default/xml/thinger.xml\"/>"
        "<include file=\"../default/xml/minibrowser.xml\"/>"
        "<include file=\"../default/xml/components.xml\"/>"
        "<include file=\"../default/xml/scripts.xml\"/>"
      "</WinampAbstractionLayer>";
    return xml;
  }

  if (!STRCMP(desc, "svc:wa2skinxml_player")) {
    int posbar_size_y, is_eq_ex=0, is_nums_ex=0;

    // check for some stuff
    SkinBitmap *posbar_bmp=new SkinBitmap("posbar.bmp");
    posbar_size_y=posbar_bmp->getHeight();
    delete(posbar_bmp);

    SkinBitmap *eq_ex_bmp=new SkinBitmap("eq_ex.bmp");
    if(eq_ex_bmp->getHeight()==82) is_eq_ex=1;
    delete(eq_ex_bmp);

    SkinBitmap *nums_ex_bmp=new SkinBitmap("nums_ex.bmp");
    if(nums_ex_bmp->getWidth()==108) is_nums_ex=1;
    delete(nums_ex_bmp);
    
    // build the xml layout
    xml=""
  "<elements>"
    "<bitmapfont id=\"player.wa2.SmallFixed\" file=\"text.bmp\" charwidth=\"5\" charheight=\"6\" hspacing=\"0\" vspacing=\"0\"/>"
    "<bitmapfont id=\"player.wa2.numbers\" file=\"numbers.bmp\" charwidth=\"12\" charheight=\"12\" hspacing=\"0\" vspacing=\"0\" onlynum=\"1\"/>"
    "<bitmapfont id=\"player.wa2.numbers.ex\" file=\"nums_ex.bmp\" charwidth=\"12\" charheight=\"12\" hspacing=\"0\" vspacing=\"0\" onlynum=\"1\"/>"
    "<bitmap id=\"player.wa2.main.bg\" file=\"main.bmp\"/>"
    "<bitmap id=\"player.wa2.eq.bg\" file=\"eqmain.bmp\" x=\"0\" y=\"0\" w=\"275\" h=\"116\"/>"
    "<bitmap id=\"player.wa2.shade.bg\" file=\"titlebar.bmp\" x=\"27\" y=\"29\" w=\"275\" h=\"14\"/>"
    "<bitmap id=\"player.wa2.eq.shade.bg\" file=\"eq_ex.bmp\" x=\"0\" y=\"0\" w=\"275\" h=\"14\"/>"
    "<bitmap id=\"player.wa2.titlebar.active\" file=\"titlebar.bmp\" x=\"27\" y=\"0\" w=\"274\" h=\"13\"/>"
    "<bitmap id=\"player.wa2.eq.titlebar.active\" file=\"eqmain.bmp\" x=\"0\" y=\"134\" w=\"274\" h=\"13\"/>"
    "<bitmap id=\"player.button.previous\" file=\"cbuttons.bmp\" x=\"0\" y=\"0\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.previous.pressed\" file=\"cbuttons.bmp\" x=\"0\" y=\"18\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.play\" file=\"cbuttons.bmp\" x=\"23\" y=\"0\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.play.pressed\" file=\"cbuttons.bmp\" x=\"23\" y=\"18\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.pause\" file=\"cbuttons.bmp\" x=\"46\" y=\"0\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.pause.pressed\" file=\"cbuttons.bmp\" x=\"46\" y=\"18\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.stop\" file=\"cbuttons.bmp\" x=\"69\" y=\"0\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.stop.pressed\" file=\"cbuttons.bmp\" x=\"69\" y=\"18\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.next\" file=\"cbuttons.bmp\" x=\"92\" y=\"0\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.next.pressed\" file=\"cbuttons.bmp\" x=\"92\" y=\"18\" h=\"18\" w=\"22\"/>"
    "<bitmap id=\"player.button.eject\" file=\"cbuttons.bmp\" x=\"114\" y=\"0\" h=\"16\" w=\"22\"/>"
    "<bitmap id=\"player.button.eject.pressed\" file=\"cbuttons.bmp\" x=\"114\" y=\"16\" h=\"16\" w=\"22\"/>"
    "<bitmap id=\"player.button.thinger\" file=\"main.bmp\" x=\"248\" y=\"89\" h=\"19\" w=\"19\"/>"
    "<bitmap id=\"player.button.sysmenu\" file=\"titlebar.bmp\" x=\"0\" y=\"0\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.sysmenu.pressed\" file=\"titlebar.bmp\" x=\"0\" y=\"9\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.minimize\" file=\"titlebar.bmp\" x=\"9\" y=\"0\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.minimize.pressed\" file=\"titlebar.bmp\" x=\"9\" y=\"9\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.shade\" file=\"titlebar.bmp\" x=\"0\" y=\"18\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.shade.pressed\" file=\"titlebar.bmp\" x=\"9\" y=\"18\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.unshade\" file=\"titlebar.bmp\" x=\"0\" y=\"27\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.unshade.pressed\" file=\"titlebar.bmp\" x=\"9\" y=\"27\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.close\" file=\"titlebar.bmp\" x=\"18\" y=\"0\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.button.close.pressed\" file=\"titlebar.bmp\" x=\"18\" y=\"9\" h=\"8\" w=\"8\"/>"
    "<bitmap id=\"player.toggler.shuffle.disabled\" file=\"shufrep.bmp\" x=\"28\" y=\"0\" h=\"15\" w=\"47\"/>"
    "<bitmap id=\"player.toggler.shuffle.pressed\" file=\"shufrep.bmp\" x=\"28\" y=\"15\" h=\"15\" w=\"47\"/>"
    "<bitmap id=\"player.toggler.shuffle.enabled\" file=\"shufrep.bmp\" x=\"28\" y=\"30\" h=\"15\" w=\"47\"/>"
    "<bitmap id=\"player.toggler.repeat.disabled\" file=\"shufrep.bmp\" x=\"0\" y=\"0\" h=\"15\" w=\"28\"/>"
    "<bitmap id=\"player.toggler.repeat.pressed\" file=\"shufrep.bmp\" x=\"0\" y=\"15\" h=\"15\" w=\"28\"/>"
    "<bitmap id=\"player.toggler.repeat.enabled\" file=\"shufrep.bmp\" x=\"0\" y=\"30\" h=\"15\" w=\"28\"/>";

    // posbar stuff
    xml+=StringPrintf("<bitmap id=\"player.slider.posbar.bg\" file=\"posbar.bmp\" x=\"0\" y=\"0\" w=\"247\" h=\"%i\"/>", posbar_size_y);
    xml+=StringPrintf("<bitmap id=\"player.slider.posbar.button\" file=\"posbar.bmp\" x=\"248\" y=\"0\" w=\"28\" h=\"%i\"/>", posbar_size_y);
    xml+=StringPrintf("<bitmap id=\"player.slider.posbar.button.pressed\" file=\"posbar.bmp\" x=\"278\" y=\"0\" w=\"28\" h=\"%i\"/>", posbar_size_y);

    xml+=

  "</elements>";

    xml+=

  // player window

  "<container id=\"main\" name=\"Main window\" default_x=\"0\" default_y=\"0\" default_visible=\"1\">"

  // normal player mode

   "<layout id=\"normal\" background=\"player.wa2.main.bg\">"

    "<layer id=\"titlebar\" x=\"0\" y=\"0\" image=\"player.wa2.titlebar.active\" dblClickAction=\"SWITCH;shade\"/>"

    "<button \
      id=\"Previous\" \
      action=\"PREV\" \
      x=\"16\" y=\"88\" \
      image=\"player.button.previous\" \
      downImage=\"player.button.previous.pressed\" \
      tooltip=\"Previous\" \
    />"
    "<button\n\
      id=\"Play\"\n\
      action=\"PLAY\"\n\
      x=\"39\" y=\"88\"\n\
      image=\"player.button.play\"\n\
      downImage=\"player.button.play.pressed\"\n\
      tooltip=\"Play\"\n\
    />\n"
    "<button\n\
      id=\"Pause\"\n\
      action=\"PAUSE\"\n\
      x=\"62\" y=\"88\"\n\
      image=\"player.button.pause\"\n\
      downImage=\"player.button.pause.pressed\"\n\
      tooltip=\"Pause\"\n\
    />\n"
    "<button\n\
      id=\"Stop\"\n\
      action=\"STOP\"\n\
      x=\"85\" y=\"88\"\n\
      image=\"player.button.stop\"\n\
      downImage=\"player.button.stop.pressed\"\n\
      tooltip=\"Stop\"\n\
    />\n"
    "<button\n\
      id=\"Next\"\n\
      action=\"NEXT\"\n\
      x=\"108\" y=\"88\"\n\
      image=\"player.button.next\"\n\
      downImage=\"player.button.next.pressed\"\n\
      tooltip=\"Next\"\n\
    />\n"
    "<button \
      id=\"Eject\" \
      action=\"EJECT\" \
      x=\"136\" y=\"89\" \
      image=\"player.button.eject\" \
      downImage=\"player.button.eject.pressed\" \
      tooltip=\"Next\" \
    />"

    "<togglebutton \
      id=\"Shuffle\" \
      x=\"164\" y=\"89\" \
      image=\"player.toggler.shuffle.disabled\" \
      downImage=\"player.toggler.shuffle.pressed\" \
      activeImage=\"player.toggler.shuffle.enabled\" \
      tooltip=\"Shuffle\" \
    />"
    "<togglebutton \
      id=\"Repeat\" \
      x=\"210\" y=\"89\" \
      image=\"player.toggler.repeat.disabled\" \
      downImage=\"player.toggler.repeat.pressed\" \
      activeImage=\"player.toggler.repeat.enabled\" \
      tooltip=\"Repeat\" \
    />"

    "<button \
      id=\"Thinger\" \
      action=\"TOGGLE\" param=\"thinger\" \
      x=\"248\" y=\"89\" \
      image=\"player.button.thinger\" \
      tooltip=\"Thinger\" \
    />"

    "<button \
      id=\"sysbutton\" \
      action=\"SYSMENU\" \
      x=\"6\" y=\"3\" \
      image=\"player.button.sysmenu\" \
      downImage=\"player.button.sysmenu.pressed\" \
      tooltip=\"Display system menu\" \
    />"
    "<button \
      id=\"Minimize\" \
      action=\"MINIMIZE\" \
      x=\"244\" y=\"3\" \
      image=\"player.button.minimize\" \
      downImage=\"player.button.minimize.pressed\" \
      tooltip=\"Minimize Winamp\" \
    />"
    "<button \
      id=\"winshade\" \
      action=\"SWITCH\" param=\"shade\" \
      x=\"254\" y=\"3\" \
      image=\"player.button.shade\" \
      downImage=\"player.button.shade.pressed\" \
      tooltip=\"Toggle Windowshade mode\" \
    />"
    "<button \
      id=\"Close\" \
      action=\"CLOSE\" \
      x=\"264\" y=\"3\" \
      image=\"player.button.close\" \
      downImage=\"player.button.close.pressed\" \
      tooltip=\"Exit Winamp\" \
    />"

/*    "<text \
      id=\"timer\" \
      display=\"time\" \
      x=\"111\" y=\"25\" \
      w=\"153\" h=\"10\" \
      font=\"player.wa2.numbers\" align=\"right\" \
      timecolonwidth=\"7\" \
    />"*/

    "<text \
      id=\"songticker\" ticker=\"1\" \
      display=\"songname\" default=\"\" showlen=\"1\" \
      x=\"111\" y=\"25\" \
      w=\"153\" h=\"10\" \
      font=\"player.wa2.SmallFixed\" \
    />"

    "<vis \
      id=\"vis\" \
      x=\"24\" y=\"43\" \
      w=\"72\" h=\"16\" \
    />"

    "<slider \
      id=\"Seeker\" \
      action=\"SEEK\" \
      x=\"16\" y=\"72\" \
      w=\"247\" h=\"10\" \
      barMiddle=\"player.slider.posbar.bg\" \
      thumb=\"player.slider.posbar.button\" \
      downThumb=\"player.slider.posbar.button.pressed\" \
    />"

   "</layout>"

  // windowshade player mode

   "<layout id=\"shade\" background=\"player.wa2.shade.bg\">"
    "<layer id=\"titlebar\" x=\"0\" y=\"0\" image=\"player.wa2.shade.bg\" dblClickAction=\"SWITCH;normal\"/>"

    "<button \
      id=\"sysbutton\" \
      action=\"SYSMENU\" \
      x=\"6\" y=\"3\" \
      image=\"player.button.sysmenu\" \
      downImage=\"player.button.sysmenu.pressed\" \
      tooltip=\"Display system menu\" \
    />"
    "<button \
      id=\"Minimize\" \
      action=\"MINIMIZE\" \
      x=\"244\" y=\"3\" \
      image=\"player.button.minimize\" \
      downImage=\"player.button.minimize.pressed\" \
      tooltip=\"Minimize Winamp\" \
    />"
    "<button \
      id=\"winshade\" \
      action=\"SWITCH\" param=\"normal\" \
      x=\"254\" y=\"3\" \
      image=\"player.button.unshade\" \
      downImage=\"player.button.unshade.pressed\" \
      tooltip=\"Toggle Windowshade mode\" \
    />"
    "<button \
      id=\"Close\" \
      action=\"CLOSE\" \
      x=\"264\" y=\"3\" \
      image=\"player.button.close\" \
      downImage=\"player.button.close.pressed\" \
      tooltip=\"Exit Winamp\" \
    />"

   "</layout>"

  "</container>"

  // equalizer window

  "<container id=\"eq\" name=\"Equalizer\" default_x=\"0\" default_y=\"116\" default_visible=\"1\">"

  // normal eq mode

   "<layout id=\"normal\" background=\"player.wa2.eq.bg\">"
    "<layer id=\"titlebar\" x=\"0\" y=\"0\" image=\"player.wa2.eq.titlebar.active\" dblClickAction=\"SWITCH;shade\"/>"

   "</layout>";

   if(is_eq_ex) {
    // windowshade eq mode
     xml+=
    "<layout id=\"shade\" background=\"player.wa2.eq.shade.bg\">"
     "<layer id=\"titlebar\" x=\"0\" y=\"0\" image=\"player.wa2.eq.shade.bg\" dblClickAction=\"SWITCH;normal\"/>"
    "</layout>";
   }

   xml+=
  "</container>";

    return xml;
  }
  return 0;
}

