#ifndef NULLSOFT_MAINH
#define NULLSOFT_MAINH
#include <windows.h>
#include "../Plugins/General/gen_ml/ml.h"
#include "../nu/MediaLibraryInterface.h"
#include "resource.h"
#include <windowsx.h>
#include "resource.h"
#include "../winamp/wa_ipc.h"
#include "../Plugins/General/gen_ml/ml.h"
#include "../Plugins/General/gen_ml/config.h"
#include "api.h"

#define WINAMP_EDIT_BOOKMARKS           40320 
extern winampMediaLibraryPlugin plugin;
INT_PTR bookmark_pluginMessageProc(int message_type, INT_PTR param1, INT_PTR param2, INT_PTR param3);
extern int bookmark_treeItem;
void bookmark_notifyAdd(wchar_t *filenametitle);

#endif