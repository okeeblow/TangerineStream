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
#include "../bfc/bitmap.h"
#include "../studio/api.h"

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
      "<WinampAbstractionLayer version=\"0.8\">"

        "<include file=\"../default/xml/studio.xml\"/>"
        "<include file=\"../default/xml/gamma-presets.xml\"/>"
        "<include file=\"svc:wa2skinxml_player\"/>"
        "<include file=\"../default/xml/pledit.xml\"/>"
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
    if(eq_ex_bmp->getHeight()/*==82*/) is_eq_ex=1;
    delete(eq_ex_bmp);

    SkinBitmap *nums_ex_bmp=new SkinBitmap("nums_ex.bmp");
    if(nums_ex_bmp->getWidth()==108) is_nums_ex=1;
    delete(nums_ex_bmp);

    SkinBitmap *volume_bmp=new SkinBitmap("volume.bmp");
    int volheight=volume_bmp->getHeight();
    int volsliderh=11;
    if(volheight>422) volsliderh=volheight-422;
    delete(volume_bmp);

    unsigned int eqpreampcol=208+(181<<8)+(156<<16);
    unsigned int eqtopcol=16+(24<<8)+(239<<16);
    unsigned int eqmidcol=239+(231<<8)+(49<<16);
    unsigned int eqbotcol=8+(156<<8)+(24<<16);
    SkinBitmap *eqmain_bmp=new SkinBitmap("eqmain.bmp");
    if(eqmain_bmp->getHeight()==315) {
      // get eq colors
      unsigned int *bits=(unsigned int *)eqmain_bmp->getBits();
      eqpreampcol=bits[314*eqmain_bmp->getWidth()+(113/2)];
      eqtopcol=bits[294*eqmain_bmp->getWidth()+115];
      eqmidcol=bits[303*eqmain_bmp->getWidth()+115];
      eqbotcol=bits[312*eqmain_bmp->getWidth()+115];
    }
    delete(eqmain_bmp);
   
    // build the xml layout
    xml=""
  "<elements>"
    "<bitmapfont id=\"player.wa2.SmallFixed\" file=\"text.bmp\" charwidth=\"5\" charheight=\"6\" hspacing=\"0\" vspacing=\"0\"/>"
    "<bitmap id=\"player.wa2.main.bg\" file=\"main.bmp\"/>"
    "<bitmap id=\"player.wa2.eq.bg\" file=\"eqmain.bmp\" x=\"0\" y=\"0\" w=\"275\" h=\"116\"/>"
    "<bitmap id=\"player.wa2.shade.bg\" file=\"titlebar.bmp\" x=\"27\" y=\"29\" w=\"275\" h=\"14\"/>"
    "<bitmap id=\"player.wa2.shade.bg.inactive\" file=\"titlebar.bmp\" x=\"27\" y=\"42\" w=\"275\" h=\"14\"/>"
    "<bitmap id=\"player.wa2.eq.shade.bg\" file=\"eq_ex.bmp\" x=\"0\" y=\"0\" w=\"275\" h=\"14\"/>"
    "<bitmap id=\"player.wa2.eq.shade.bg.inactive\" file=\"eq_ex.bmp\" x=\"0\" y=\"15\" w=\"275\" h=\"14\"/>"
    "<bitmap id=\"player.wa2.titlebar.active\" file=\"titlebar.bmp\" x=\"27\" y=\"0\" w=\"274\" h=\"13\"/>"
    "<bitmap id=\"player.wa2.titlebar.inactive\" file=\"titlebar.bmp\" x=\"27\" y=\"15\" w=\"274\" h=\"13\"/>"
    "<bitmap id=\"player.wa2.eq.titlebar.active\" file=\"eqmain.bmp\" x=\"0\" y=\"134\" w=\"274\" h=\"13\"/>"
    "<bitmap id=\"player.wa2.eq.titlebar.inactive\" file=\"eqmain.bmp\" x=\"0\" y=\"149\" w=\"274\" h=\"13\"/>"
    "<bitmap id=\"player.wa2.eq.slider\" file=\"eqmain.bmp\" x=\"13\" y=\"164\" w=\"210\" h=\"128\"/>"
    "<bitmap id=\"player.slider.volbar\" file=\"volume.bmp\" x=\"0\" y=\"0\" w=\"68\" h=\"422\"/>"
    "<bitmap id=\"player.slider.balance\" file=\"balance.bmp\" x=\"9\" y=\"0\" w=\"38\" h=\"422\"/>"
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
    "<bitmap id=\"player.toggler.repeat.enabled\" file=\"shufrep.bmp\" x=\"0\" y=\"30\" h=\"15\" w=\"28\"/>"
    "<bitmap id=\"player.toggler.eq.disabled\" file=\"shufrep.bmp\" x=\"0\" y=\"61\" h=\"12\" w=\"23\"/>"
    "<bitmap id=\"player.toggler.eq.pressed\" file=\"shufrep.bmp\" x=\"46\" y=\"73\" h=\"12\" w=\"23\"/>"
    "<bitmap id=\"player.toggler.eq.enabled\" file=\"shufrep.bmp\" x=\"0\" y=\"73\" h=\"12\" w=\"23\"/>"
    "<bitmap id=\"player.toggler.pl.disabled\" file=\"shufrep.bmp\" x=\"23\" y=\"61\" h=\"12\" w=\"23\"/>"
    "<bitmap id=\"player.toggler.pl.pressed\" file=\"shufrep.bmp\" x=\"69\" y=\"73\" h=\"12\" w=\"23\"/>"
    "<bitmap id=\"player.toggler.pl.enabled\" file=\"shufrep.bmp\" x=\"23\" y=\"73\" h=\"12\" w=\"23\"/>"
    "<bitmap id=\"player.status.play\" file=\"playpaus.bmp\" x=\"1\" y=\"0\" h=\"9\" w=\"8\"/>"
    "<bitmap id=\"player.status.pause\" file=\"playpaus.bmp\" x=\"9\" y=\"0\" h=\"9\" w=\"9\"/>"
    "<bitmap id=\"player.status.stop\" file=\"playpaus.bmp\" x=\"18\" y=\"0\" h=\"9\" w=\"9\"/>"
    "<bitmap id=\"player.status.ok\" file=\"playpaus.bmp\" x=\"36\" y=\"0\" h=\"9\" w=\"3\"/>"
    "<bitmap id=\"player.status.mono.active\" file=\"monoster.bmp\" x=\"29\" y=\"0\" h=\"12\" w=\"27\"/>"
    "<bitmap id=\"player.status.mono.inactive\" file=\"monoster.bmp\" x=\"29\" y=\"12\" h=\"12\" w=\"27\"/>"
    "<bitmap id=\"player.status.stereo.active\" file=\"monoster.bmp\" x=\"0\" y=\"0\" h=\"12\" w=\"29\"/>"
    "<bitmap id=\"player.status.stereo.inactive\" file=\"monoster.bmp\" x=\"0\" y=\"12\" h=\"12\" w=\"29\"/>"
    "<bitmap id=\"player.eq.toggler.on.disabled\" file=\"eqmain.bmp\" x=\"10\" y=\"119\" h=\"12\" w=\"24\"/>"
    "<bitmap id=\"player.eq.toggler.on.pressed\" file=\"eqmain.bmp\" x=\"128\" y=\"119\" h=\"12\" w=\"24\"/>"
    "<bitmap id=\"player.eq.toggler.on.enabled\" file=\"eqmain.bmp\" x=\"69\" y=\"119\" h=\"12\" w=\"24\"/>"
    "<bitmap id=\"player.eq.toggler.auto.disabled\" file=\"eqmain.bmp\" x=\"34\" y=\"119\" h=\"12\" w=\"34\"/>"
    "<bitmap id=\"player.eq.toggler.auto.pressed\" file=\"eqmain.bmp\" x=\"152\" y=\"119\" h=\"12\" w=\"34\"/>"
    "<bitmap id=\"player.eq.toggler.auto.enabled\" file=\"eqmain.bmp\" x=\"93\" y=\"119\" h=\"12\" w=\"34\"/>"
    "<bitmap id=\"player.eq.button.presets\" file=\"eqmain.bmp\" x=\"224\" y=\"164\" h=\"12\" w=\"44\"/>"
    "<bitmap id=\"player.eq.button.presets.pressed\" file=\"eqmain.bmp\" x=\"224\" y=\"176\" h=\"12\" w=\"44\"/>"
    "<bitmap id=\"player.eq.button.close\" file=\"eqmain.bmp\" x=\"0\" y=\"116\" h=\"9\" w=\"9\"/>"
    "<bitmap id=\"player.eq.button.close.pressed\" file=\"eqmain.bmp\" x=\"0\" y=\"125\" h=\"9\" w=\"9\"/>"
    "<bitmap id=\"player.eq.button.bandslider\" file=\"eqmain.bmp\" x=\"0\" y=\"164\" h=\"11\" w=\"11\"/>"
    "<bitmap id=\"player.eq.button.bandslider.pressed\" file=\"eqmain.bmp\" x=\"0\" y=\"176\" h=\"11\" w=\"11\"/>"
    "<bitmap id=\"player.shade.button.previous\" file=\"titlebar.bmp\" x=\"193\" y=\"32\" h=\"11\" w=\"10\"/>"
    "<bitmap id=\"player.shade.button.play\" file=\"titlebar.bmp\" x=\"203\" y=\"32\" h=\"11\" w=\"10\"/>"
    "<bitmap id=\"player.shade.button.pause\" file=\"titlebar.bmp\" x=\"213\" y=\"32\" h=\"11\" w=\"10\"/>"
    "<bitmap id=\"player.shade.button.stop\" file=\"titlebar.bmp\" x=\"223\" y=\"32\" h=\"11\" w=\"10\"/>"
    "<bitmap id=\"player.shade.button.next\" file=\"titlebar.bmp\" x=\"233\" y=\"32\" h=\"11\" w=\"10\"/>"
    "<bitmap id=\"player.shade.button.eject\" file=\"titlebar.bmp\" x=\"243\" y=\"32\" h=\"11\" w=\"10\"/>"
    "<bitmap id=\"player.shade.slider.volume\" file=\"eq_ex.bmp\" x=\"4\" y=\"30\" h=\"7\" w=\"3\"/>"
    "<bitmap id=\"player.shade.slider.pan\" file=\"eq_ex.bmp\" x=\"14\" y=\"30\" h=\"7\" w=\"3\"/>"
    "<bitmap id=\"player.shade.slider.seek\" file=\"titlebar.bmp\" x=\"20\" y=\"36\" h=\"7\" w=\"3\"/>"
    "<bitmap id=\"player.eq.shade.button.shade\" file=\"eq_ex.bmp\" x=\"254\" y=\"3\" h=\"9\" w=\"9\"/>"
    "<bitmap id=\"player.eq.shade.button.shade.pressed\" file=\"eq_ex.bmp\" x=\"1\" y=\"47\" h=\"9\" w=\"9\"/>"
    "<bitmap id=\"player.eq.shade.button.close\" file=\"eq_ex.bmp\" x=\"11\" y=\"38\" h=\"9\" w=\"9\"/>"
    "<bitmap id=\"player.eq.shade.button.close.pressed\" file=\"eq_ex.bmp\" x=\"11\" y=\"47\" h=\"9\" w=\"9\"/>"
    "<bitmap id=\"wa2.component.topleft\" file=\"pledit.bmp\" x=\"0\" y=\"0\" h=\"20\" w=\"25\"/>"
    "<bitmap id=\"wa2.component.topright\" file=\"pledit.bmp\" x=\"153\" y=\"0\" h=\"20\" w=\"25\"/>"
    "<bitmap id=\"wa2.component.left\" file=\"pledit.bmp\" x=\"0\" y=\"42\" h=\"29\" w=\"25\"/>"
    "<bitmap id=\"wa2.component.bottomleft\" file=\"pledit.bmp\" x=\"0\" y=\"98\" h=\"12\" w=\"125\"/>"
    "<bitmap id=\"wa2.component.bottomleftline\" file=\"pledit.bmp\" x=\"0\" y=\"72\" h=\"7\" w=\"125\"/>"
    "<bitmap id=\"wa2.component.bottomright\" file=\"pledit.bmp\" x=\"254\" y=\"84\" h=\"25\" w=\"21\"/>";

    // posbar stuff
    xml+=StringPrintf("<bitmap id=\"player.slider.posbar.bg\" file=\"posbar.bmp\" x=\"0\" y=\"0\" w=\"247\" h=\"%i\"/>", posbar_size_y);
    xml+=StringPrintf("<bitmap id=\"player.slider.posbar.button\" file=\"posbar.bmp\" x=\"248\" y=\"0\" w=\"28\" h=\"%i\"/>", posbar_size_y);
    xml+=StringPrintf("<bitmap id=\"player.slider.posbar.button.pressed\" file=\"posbar.bmp\" x=\"278\" y=\"0\" w=\"28\" h=\"%i\"/>", posbar_size_y);

    // volbar stuff
    xml+=StringPrintf("<bitmap id=\"player.slider.volbar.button\" file=\"volume.bmp\" x=\"15\" y=\"422\" w=\"14\" h=\"%i\"/>", volsliderh);
    xml+=StringPrintf("<bitmap id=\"player.slider.volbar.button.pressed\" file=\"volume.bmp\" x=\"0\" y=\"422\" w=\"14\" h=\"%i\"/>", volsliderh);

    // numbers
    xml+=StringPrintf("<bitmap id=\"player.wa2.numbers\" file=\"%s\"/>",is_nums_ex?"nums_ex.bmp":"numbers.bmp");

    xml+=

  "</elements>";

    xml+=

  // player window

  "<container id=\"main\" name=\"Main window\" default_x=\"0\" default_y=\"0\" default_visible=\"1\">"

  // normal player mode

   "<layout id=\"normal\" background=\"player.wa2.main.bg\">"

    "<layer id=\"titlebar\" x=\"0\" y=\"0\" image=\"player.wa2.titlebar.active\" dblClickAction=\"SWITCH;shade\" inactiveImage=\"player.wa2.titlebar.inactive\" />"

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
      tooltip=\"Open file\" \
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

    "<button \
      id=\"eq\" \
      action=\"TOGGLE\" param=\"eq\" \
      x=\"219\" y=\"58\" \
      image=\"player.toggler.eq.disabled\" \
      downImage=\"player.toggler.eq.pressed\" \
      activeImage=\"player.toggler.eq.enabled\" \
      tooltip=\"Toggle Equalizer\" \
    />"
    "<button \
      id=\"pl\" \
      action=\"TOGGLE\" param=\"guid:pl\" \
      x=\"242\" y=\"58\" \
      image=\"player.toggler.pl.disabled\" \
      downImage=\"player.toggler.pl.pressed\" \
      activeImage=\"player.toggler.pl.enabled\" \
      tooltip=\"Toggle Playlist Editor\" \
    />"
    
    "<wa2time id=\"timer\" x=\"36\" y=\"26\" w=\"65\" h=\"13\" numbers=\"player.wa2.numbers\" />"

    "<text \
      id=\"songticker\" ticker=\"1\" \
      display=\"songname\" default=\"\" showlen=\"1\" \
      x=\"111\" y=\"25\" \
      w=\"153\" h=\"10\" \
      font=\"player.wa2.SmallFixed\" \
    />"

    "<text \
      id=\"bitrate\" ticker=\"0\" \
      display=\"songbitrate\" default=\"\" \
      x=\"109\" y=\"41\" \
      w=\"20\" h=\"11\" \
      font=\"player.wa2.SmallFixed\" align=\"left\" \
    />"

    "<text \
      id=\"srate\" ticker=\"0\" \
      display=\"songsamplerate\" default=\"\" \
      x=\"154\" y=\"41\" \
      w=\"20\" h=\"11\" \
      font=\"player.wa2.SmallFixed\" align=\"left\" \
    />"

    "<vis \
      id=\"vis\" \
      x=\"24\" y=\"43\" \
      w=\"72\" h=\"16\" ";

    // vis colors
    String viscolors;

    FILE *fh=FOPEN(StringPrintf("%sviscolor.txt",api->getSkinPath()),"rt");
    if(fh) {
      for(int x=0;x<24;x++) {
        int t;
				char progdir[91],*p=progdir;
				if(!FGETS(progdir,90,fh)) break;
        int vals[3]={0,};
			  for (t=0; t<3; t ++) {
				  int b=0,s=0;
					while (*p == ' ' || *p == ',' || *p == '\t') p++;
					while (*p >= '0' && *p <= '9') {s=1;b=b*10+*p++-'0';}
					if (!s) { x=24; break; }
					vals[t]=b;
				}
        if(x>=2 && x<=17) viscolors+=StringPrintf("colorband%i=\"%i,%i,%i\" ",(16-(x-2)),vals[0],vals[1],vals[2]);
        if(x>=18 && x<=22) viscolors+=StringPrintf("colorosc%i=\"%i,%i,%i\" ",x-17,vals[0],vals[1],vals[2]);
        if(x==23) viscolors+=StringPrintf("colorbandpeak=\"%i,%i,%i\" ",vals[0],vals[1],vals[2]);
      }
      FCLOSE(fh);
    }

    xml+=viscolors;

    xml+= 
    
    "/>"

    "<slider \
      id=\"Seeker\" \
      action=\"SEEK\" \
      x=\"16\" y=\"72\" \
      w=\"247\" h=\"10\" \
      barMiddle=\"player.slider.posbar.bg\" \
      thumb=\"player.slider.posbar.button\" \
      downThumb=\"player.slider.posbar.button.pressed\" \
    />"

    "<status \
      id=\"status\" \
      x=\"24\" y=\"28\" \
      w=\"9\" h=\"9\" \
      stopBitmap=\"player.status.stop\" \
      playBitmap=\"player.status.play\" \
      pauseBitmap=\"player.status.pause\" \
    />"
    //"<layer id=\"stat\" x=\"21\" y=\"28\" image=\"player.status.ok\" />"

    "<Wa2Slider id=\"volumewa2\" action=\"volume\" x=\"108\" y=\"58\" w=\"68\" h=\"13\" \
      images=\"player.slider.volbar\" imagesspacing=\"15\" \
      thumb=\"player.slider.volbar.button\" \
      downThumb=\"player.slider.volbar.button.pressed\" \
    />"

    "<Wa2Slider id=\"balancewa2\" action=\"balance\" x=\"177\" y=\"58\" w=\"38\" h=\"13\" \
      images=\"player.slider.balance\" imagesspacing=\"15\" \
      thumb=\"player.slider.volbar.button\" \
      downThumb=\"player.slider.volbar.button.pressed\" \
    />"
    
    "<Wa2Ms id=\"mono\" x=\"212\" y=\"41\" w=\"27\" h=\"12\" active=\"player.status.mono.active\" inactive=\"player.status.mono.inactive\" string=\"mono\" />"
    "<Wa2Ms id=\"stereo\" x=\"239\" y=\"41\" w=\"29\" h=\"12\" active=\"player.status.stereo.active\" inactive=\"player.status.stereo.inactive\" string=\"stereo\" />"
    
   "</layout>"

  // windowshade player mode

   "<layout id=\"shade\" background=\"player.wa2.shade.bg\">"
    "<layer id=\"titlebar\" x=\"0\" y=\"0\" image=\"player.wa2.shade.bg\" dblClickAction=\"SWITCH;normal\" inactiveImage=\"player.wa2.shade.bg.inactive\" />"

    // buttons
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

    "<button \
      id=\"Previous\" \
      action=\"PREV\" \
      x=\"166\" y=\"3\" \
      image=\"player.shade.button.previous\" \
      downImage=\"player.shade.button.previous\" \
      tooltip=\"Previous\" \
    />"
    "<button \
      id=\"Play\" \
      action=\"PLAY\" \
      x=\"176\" y=\"3\" \
      image=\"player.shade.button.play\" \
      downImage=\"player.shade.button.play\" \
      tooltip=\"Play\" \
    />"
    "<button \
      id=\"Pause\" \
      action=\"PAUSE\" \
      x=\"186\" y=\"3\" \
      image=\"player.shade.button.pause\" \
      downImage=\"player.shade.button.pause\" \
      tooltip=\"Pause\" \
    />"
    "<button \
      id=\"Stop\" \
      action=\"STOP\" \
      x=\"196\" y=\"3\" \
      image=\"player.shade.button.stop\" \
      downImage=\"player.shade.button.stop\" \
      tooltip=\"Stop\" \
    />"
    "<button \
      id=\"Next\" \
      action=\"NEXT\" \
      x=\"206\" y=\"3\" \
      image=\"player.shade.button.next\" \
      downImage=\"player.shade.button.next\" \
      tooltip=\"Next\" \
    />"
    "<button \
      id=\"Eject\" \
      action=\"EJECT\" \
      x=\"216\" y=\"3\" \
      image=\"player.shade.button.eject\" \
      downImage=\"player.shade.button.eject\" \
      tooltip=\"Eject\" \
    />"

    // text
    "<text \
      id=\"timer\" \
      display=\"time\" \
      x=\"120\" y=\"2\" \
      w=\"36\" h=\"10\" \
      font=\"player.wa2.SmallFixed\" fontsize=\"8\" align=\"right\" \
    />"
    
    "<vis \
      id=\"vis\" \
      x=\"79\" y=\"5\" \
      w=\"38\" h=\"5\" ";
   xml+=viscolors;
   xml+=
    "/>"

    "<slider \
      id=\"seeker\" \
      action=\"SEEK\" \
      x=\"227\" y=\"4\" \
      w=\"15\" h=\"7\" \
      thumb=\"player.shade.slider.seek\" \
      downThumb=\"player.shade.slider.seek\" \
    />"

   "</layout>"

  "</container>"

  // equalizer window

  "<container id=\"eq\" name=\"Equalizer\" default_x=\"0\" default_y=\"116\" default_visible=\"1\">"

  // normal eq mode

   "<layout id=\"normal\" background=\"player.wa2.eq.bg\">"
    "<layer id=\"titlebar\" x=\"0\" y=\"0\" image=\"player.wa2.eq.titlebar.active\" dblClickAction=\"SWITCH;shade\" inactiveImage=\"player.wa2.eq.titlebar.inactive\" />"

    // buttons
    "<button \
      id=\"shade\" \
      action=\"SWITCH\" param=\"shade\" \
      x=\"254\" y=\"3\" \
      image=\"player.button.shade\" \
      downImage=\"player.button.shade.pressed\" \
      tooltip=\"Toggle Windowshade mode\" \
    />"

    "<button \
      id=\"toggle\" \
      action=\"EQ_TOGGLE\" \
      x=\"14\" y=\"18\" \
      image=\"player.eq.toggler.on.disabled\" \
      downImage=\"player.eq.toggler.on.pressed\" \
      activeImage=\"player.eq.toggler.on.enabled\" \
      tooltip=\"Toggle EQ processing\" \
    />"
    "<button \
      id=\"auto\" \
      action=\"EQ_AUTO\" \
      x=\"38\" y=\"18\" \
      image=\"player.eq.toggler.auto.disabled\" \
      downImage=\"player.eq.toggler.auto.pressed\" \
      activeImage=\"player.eq.toggler.auto.enabled\" \
      tooltip=\"Toggle EQ automatic processing\" \
    />"
    "<button \
      id=\"presets\" \
      action=\"MENU\" param=\"presets\" \
      x=\"217\" y=\"18\" \
      image=\"player.eq.button.presets\" \
      downImage=\"player.eq.button.presets.pressed\" \
      tooltip=\"Display Presets menu\" \
    />"

    "<button \
      id=\"close\" \
      action=\"CLOSE\" \
      x=\"264\" y=\"3\" \
      image=\"player.eq.button.close\" \
      downImage=\"player.eq.button.close.pressed\" \
      tooltip=\"Close EQ window\" \
    />";

    // spline vis
    xml+=StringPrintf("<eqvis \
      id=\"eq_spline\" \
      x=\"86\" y=\"17\" \
      w=\"113\" h=\"19\" \
      colortop=\"%i,%i,%i\" \
      colormiddle=\"%i,%i,%i\" \
      colorbottom=\"%i,%i,%i\" \
      colorpreamp=\"%i,%i,%i\" \
      gamma=\"192\" \
    />"
    ,(eqtopcol>>16)&0xff,(eqtopcol>>8)&0xff,eqtopcol&0xff
    ,(eqmidcol>>16)&0xff,(eqmidcol>>8)&0xff,eqmidcol&0xff
    ,(eqbotcol>>16)&0xff,(eqbotcol>>8)&0xff,eqbotcol&0xff
    ,(eqpreampcol>>16)&0xff,(eqpreampcol>>8)&0xff,eqpreampcol&0xff);

    xml +=
    // sliders
    "<wa2eqslider \
       id=\"preamp\" \
       band=\"0\" \
       x=\"21\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"

    "<wa2eqslider \
       id=\"eq1\" \
       band=\"1\" \
       x=\"78\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    // x should be 98 but wa2 uses 96, so lets duplicate the bug
    "<wa2eqslider \
       id=\"eq2\" \
       band=\"2\" \
       x=\"96\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    "<wa2eqslider \
       id=\"eq3\" \
       band=\"3\" \
       x=\"114\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    "<wa2eqslider \
       id=\"eq4\" \
       band=\"4\" \
       x=\"132\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    "<wa2eqslider \
       id=\"eq5\" \
       band=\"5\" \
       x=\"150\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    "<wa2eqslider \
       id=\"eq6\" \
       band=\"6\" \
       x=\"168\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    "<wa2eqslider \
       id=\"eq7\" \
       band=\"7\" \
       x=\"186\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    "<wa2eqslider \
       id=\"eq8\" \
       band=\"8\" \
       x=\"204\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    "<wa2eqslider \
       id=\"eq9\" \
       band=\"9\" \
       x=\"222\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    "<wa2eqslider \
       id=\"eq10\" \
       band=\"10\" \
       x=\"240\" y=\"38\" \
       w=\"14\" h=\"63\" \
    />"
    
   "</layout>";

   if(is_eq_ex) {
    // windowshade eq mode
     xml+=
    "<layout id=\"shade\" background=\"player.wa2.eq.shade.bg\">"
     "<layer id=\"titlebar\" x=\"0\" y=\"0\" image=\"player.wa2.eq.shade.bg\" dblClickAction=\"SWITCH;normal\" inactiveImage=\"player.wa2.eq.shade.bg.inactive\" />"

     "<slider \
      id=\"volume\" \
      action=\"VOLUME\" \
      x=\"61\" y=\"3\" \
      w=\"96\" h=\"10\" \
      thumb=\"player.shade.slider.volume\" \
      downThumb=\"player.shade.slider.volume\" \
     />"

     "<slider \
      id=\"balance\" \
      action=\"PAN\" \
      x=\"164\" y=\"3\" \
      w=\"42\" h=\"10\" \
      thumb=\"player.shade.slider.pan\" \
      downThumb=\"player.shade.slider.pan\" \
     />"

    "<button \
      id=\"winshade\" \
      action=\"SWITCH\" param=\"normal\" \
      x=\"254\" y=\"3\" \
      image=\"player.eq.shade.button.shade\" \
      downImage=\"player.eq.shade.button.shade.pressed\" \
      tooltip=\"Toggle Windowshade mode\" \
    />"

    "<button \
      id=\"close\" \
      action=\"CLOSE\" \
      x=\"264\" y=\"3\" \
      image=\"player.eq.shade.button.close\" \
      downImage=\"player.eq.shade.button.close.pressed\" \
      tooltip=\"Close Equalizer\" \
    />"

    "</layout>";
   }

   xml+=
  "</container>"
  ;

   // standard frame
  //xml+=

    "<groupdef id=\"studio.standardframe\" >"  //background=\"component.basetexture\"

    "<layer \
      id=\"topleftcorner\" \
      x=\"0\" \
      y=\"0\" \
      image=\"wa2.component.topleft\" \
      resize=\"topleft\" \
    />"

    "<layer \
      id=\"toprightcorner\" \
      x=\"-25\" \
      relatx=\"1\" \
      y=\"0\" \
      image=\"wa2.component.topright\" \
      resize=\"topright\" \
    />"

    "<layer \
      id=\"bottomleftcorner\" \
      x=\"0\" \
      y=\"-12\" \
      relaty=\"1\" \
      image=\"wa2.component.bottomleft\" \
      resize=\"bottomleft\" \
    />"
    "<layer \
      id=\"bottomleftcornerline\" \
      x=\"0\" \
      y=\"-19\" \
      relaty=\"1\" \
      image=\"wa2.component.bottomleftline\" \
      resize=\"bottomleft\" \
    />"

    "<layer \
      id=\"bottomrightcorner\" \
      x=\"-21\" \
      relatx=\"1\" \
      y=\"-25\" \
      relaty=\"1\" \
      image=\"wa2.component.bottomright\" \
      resize=\"bottomright\" \
    />"

    "<layer \
      id=\"left\" \
      x=\"0\" \
      y=\"20\" \
      h=\"-39\" \
      relath=\"1\" \
      image=\"wa2.component.left\" \
      resize=\"left\" \
    />"

    "<titlebar \
      id=\"titlebar\" \
      x=\"22\" \
      y=\"2\" \
      w=\"-43\" \
      relatw=\"1\" \
      h=\"11.5\" \
      title=\":componentname\" \
    />"

    //"<script id=\"standardframe.script\" file=\"scripts/standardframe.maki\" param=\"5,15,-10,-34,0,0,1,1\" />"

    "</groupdef>";

    return xml;
  }
  return 0;
}

