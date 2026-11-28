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
#include "guiobjects.h"
#include "../studio/services/svc_tooltips.h"
#include "../studio/services/creators.h"

static WACNAME wac;
WAComponentClient *the = &wac;
WACNAME *wacguiobjs = &wac;

#include "../studio/services/servicei.h"

#include "svc.h"
#include "grouptips.h"
#ifdef WASABI_EXTERNAL_GUIOBJECTS
#include "xuibrowser.h"
#include "xuiframe.h"
#include "xuigrid.h"
#include "xuiquerylist.h"
#include "xuifilterlist.h"
#include "xuiqueryline.h"
#include "xuiwndholder.h"
#include "xuistatus.h"
#include "xuitabsheet.h"
#include "xuicheckbox.h"
#include "xuititlebox.h"
#include "xuicustomobject.h"
#include "xuiradiogroup.h"
#include "xuihideobject.h"
#include "xuisendparams.h"
#include "xuiaddparams.h"
#include "xuilist.h"
#include "xuitree.h"
#include "xuidropdownlist.h"
#include "xuicombobox.h"
#include "xuihistoryedit.h"
#include "xuiobjdirwnd.h"
#include "xuirect.h"
#include "xuipathpicker.h"
#include "xuigradientwnd.h"
#include "xuiquerydrag.h"
#include "xuiformwnd.h"
#endif // WASABI_EXTERNAL_GUIOBJECTS
#include "multelementsvc.h"
#include "feed_svcs.h"
#include "../bfc/blending.h"

#include "../studio/runlevelcb.h"

// {9420C66F-75F7-4850-9250-D0DEA57DA068}
static const GUID guid = 
{ 0x9420c66f, 0x75f7, 0x4850, { 0x92, 0x50, 0xd0, 0xde, 0xa5, 0x7d, 0xa0, 0x68 } };

WACNAME::WACNAME() {
  registerSkinFile("wasabi/wasabi.xml", RSF_RELATIVETOTHEME);
  registerService(new waServiceFactoryT<svc_toolTipsRenderer, GroupTips>);
  registerService(new waServiceFactoryT<svc_xmlProvider, MultipleElementsXmlProvider>);
#ifdef WASABI_EXTERNAL_GUIOBJECTS
  registerService(new ScriptObjectCreator<ScriptObjectSvc>);
  registerService(new XuiObjectCreator<BrowserXuiSvc>);
  registerService(new XuiObjectCreator<FrameXuiSvc>);
  registerService(new XuiObjectCreator<GridXuiSvc>);
  registerService(new XuiObjectCreator<QueryListXuiSvc>);
  registerService(new XuiObjectCreator<QueryLineXuiSvc>);
  registerService(new XuiObjectCreator<FilterListXuiSvc>);
  registerService(new XuiObjectCreator<WindowHolderXuiSvc>);
  registerService(new XuiObjectCreator<WindowHolderXuiSvc2>);
  registerService(new XuiObjectCreator<ScriptObjDirWndXuiSvc>);
  registerService(new XuiObjectCreator<LayoutStatusXuiSvc>);
  registerService(new XuiObjectCreator<ScriptTabSheetXuiSvc>);
  registerService(new XuiObjectCreator<ScriptCheckBoxXuiSvc>);
  registerService(new XuiObjectCreator<ScriptTitleBoxXuiSvc>);
  registerService(new XuiObjectCreator<CustomObjectXuiSvc>);
  registerService(new XuiObjectCreator<ScriptRadioGroupXuiSvc>);
  registerService(new XuiObjectCreator<HideObjectXuiSvc>);
  registerService(new XuiObjectCreator<SendParamsXuiSvc>);
  registerService(new XuiObjectCreator<AddParamsXuiSvc>);
  registerService(new XuiObjectCreator<ScriptListXuiSvc>);
  registerService(new XuiObjectCreator<ScriptTreeXuiSvc>);
  registerService(new XuiObjectCreator<DropDownListXuiSvc>);
  registerService(new XuiObjectCreator<ComboBoxXuiSvc>);
  registerService(new XuiObjectCreator<HistoryEditXuiSvc>);
  registerService(new XuiObjectCreator<ScriptRectXuiSvc>);
  registerService(new XuiObjectCreator<PathPickerXuiSvc>);
  registerService(new XuiObjectCreator<GradientWndXuiSvc>);
  registerService(new XuiObjectCreator<QueryDragXuiSvc>);
  registerService(new XuiObjectCreator<XuiFormWndXuiSvc>);
#endif

  registerSkinFile("msgbox/msgbox.xml", RSF_RELATIVETOTHEME);
  registerSkinFile("tooltips/tooltips.xml", RSF_RELATIVETOTHEME);

  registerSkinFile("tabsheet/tabsheet.xml", RSF_RELATIVETOTHEME);
  registerSkinFile("checkbox/checkbox.xml", RSF_RELATIVETOTHEME);
  registerSkinFile("titlebox/titlebox.xml", RSF_RELATIVETOTHEME);
  registerSkinFile("dropdownlist/dropdownlist.xml", RSF_RELATIVETOTHEME);
  registerSkinFile("combobox/combobox.xml", RSF_RELATIVETOTHEME);
  registerSkinFile("historyeditbox/historyeditbox.xml", RSF_RELATIVETOTHEME);
  registerSkinFile("pathpicker/pathpicker.xml", RSF_RELATIVETOTHEME);
  registerSkinFile("statusbar/statusbar.xml", RSF_RELATIVETOTHEME);
//  registerSkinFile("xml/popupmenu/popupmenu.xml");
}

WACNAME::~WACNAME() {
}

GUID WACNAME::getGUID() {
  return guid;
}

class FeedReg : public RunlevelCallbackI {
public:
  virtual void runlevelcb_onAppRunning() {
    api->service_register(new waServiceFactoryTSingle<svc_textFeed, FeedServices>);
  }
};
FeedReg feedreg;

void WACNAME::onCreate() {
  //CT> this needs all components to be loaded first
  //api->service_register(new waServiceFactoryTSingle<svc_textFeed, FeedServices>);
  api->syscb_registerCallback(&feedreg);
}

void WACNAME::onDestroy() {
  api->syscb_deregisterCallback(&feedreg);
}

int WACNAME::onNotify(int cmd, int param1, int param2, int param3, int param4) {
  switch (cmd) {
    case WAC_NOTIFY_LOADEARLY: return 1;
  }
  return 0;
}
