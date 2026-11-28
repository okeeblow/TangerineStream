// Tangerine Stream supplement to Nullsoft's Winamp 2.90 SDK. See the project's LICENSE.
// Source-compatible navigation helpers for the original, ID-based Media Library.
// These handles are local ID tokens. Never send them as host pointers.
#ifndef TANGERINE_WINAMP290_ML_IPC_0313_H
#define TANGERINE_WINAMP290_ML_IPC_0313_H

#include <windows.h>
#include "ml.h"
#include <list>
#include <new>
#include <vector>

#define SENDMLIPC(hwnd, message, param) SendMessageW((hwnd), WM_ML_IPC, (WPARAM)(param), (LPARAM)(message))
#define ML_IPC_GETVERSION 0x0000
#define GetMLVersion(hwnd) ((DWORD)SENDMLIPC((hwnd), ML_IPC_GETVERSION, 0))

typedef struct ML290TreeItemToken *HNAVITEM;
#define NCI_LAST ((HNAVITEM)0)
#define NIMF_ITEMID 0x0001
#define NIMF_TEXT 0x0002
#define NIMF_TEXTINVARIANT 0x0004
#define NIMF_STYLE 0x0040
#define NIS_HASCHILDREN 0x0001
#define NIS_DEFAULTIMAGE 0x0080

struct NAVITEM {
    INT cbSize;
    HNAVITEM hItem;
    UINT mask;
    INT id;
    LPWSTR pszText;
    INT cchTextMax;
    LPWSTR pszInvariant;
    INT cchInvariantMax;
    INT iImage, iSelectedImage;
    UINT state, stateMask, style, styleMask;
    HFONT hFont;
    LPARAM lParam;
};

struct NAVINSERTSTRUCT {
    HNAVITEM hParent, hInsertAfter;
    NAVITEM item;
};

namespace ml290_detail {
// The 2.90 API has no item lookup. Remember the items this plug-in creates,
// in insertion order, on the Media Library UI thread.
enum ItemStatus { live, deleting, retired };
struct CreatedItem {
    HWND library;
    int id;
    CreatedItem *parent;
    ItemStatus status;
};

inline std::list<CreatedItem>& createdItems() {
    static std::list<CreatedItem> items;
    return items;
}

inline CreatedItem *findCreated(HWND library, int id) {
    std::list<CreatedItem>& items = createdItems();
    for (std::list<CreatedItem>::iterator it = items.begin(); it != items.end(); ++it) {
        if (it->library == library && it->id == id) return &*it;
    }
    return 0;
}

inline HNAVITEM token(int id) {
    return id > 0 ? reinterpret_cast<HNAVITEM>(static_cast<INT_PTR>(id)) : 0;
}

inline int id(HNAVITEM item) {
    return static_cast<int>(reinterpret_cast<INT_PTR>(item));
}
}

inline HNAVITEM MLNavCtrl_GetSelection(HWND library) {
    return ml290_detail::token(static_cast<int>(SENDMLIPC(library, ML_IPC_GETCURTREEITEM, 0)));
}

// Arbitrary items owned by other plug-ins can only be identified while selected.
inline HNAVITEM MLNavCtrl_FindItemById(HWND library, int id) {
    HNAVITEM item = ml290_detail::token(id);
    if (!item) return 0;
    if (ml290_detail::findCreated(library, id) || MLNavCtrl_GetSelection(library) == item) return item;
    return 0;
}

inline HNAVITEM MLNavCtrl_InsertItem(HWND library, const NAVINSERTSTRUCT *insert) {
    if (!library || !insert || insert->hInsertAfter != NCI_LAST) return 0;
    const NAVITEM& item = insert->item;
    if (item.cbSize != sizeof(NAVITEM) || !(item.mask & NIMF_TEXT) || !item.pszText ||
        (item.mask & ~(NIMF_TEXT | NIMF_TEXTINVARIANT | NIMF_STYLE))) return 0;
    if ((item.mask & NIMF_STYLE) && (item.styleMask & ~(NIS_HASCHILDREN | NIS_DEFAULTIMAGE))) return 0;
    const int bytes = WideCharToMultiByte(CP_ACP, 0, item.pszText, -1, 0, 0, 0, 0);
    if (!bytes) return 0;
    try {
        std::vector<char> title(bytes);
        if (!WideCharToMultiByte(CP_ACP, 0, item.pszText, -1, &title[0], bytes, 0, 0)) return 0;
        mlAddTreeItemStruct legacy = {};
        legacy.parent_id = ml290_detail::id(insert->hParent);
        legacy.title = &title[0];
        legacy.has_children = (item.mask & NIMF_STYLE) && (item.style & item.styleMask & NIS_HASCHILDREN);
        // Reserve the bookkeeping node before the host creates the item.
        std::list<ml290_detail::CreatedItem>& items = ml290_detail::createdItems();
        std::list<ml290_detail::CreatedItem> pending(1);
        SENDMLIPC(library, ML_IPC_ADDTREEITEM, &legacy);
        if (legacy.this_id <= 0) return 0;
        ml290_detail::CreatedItem record = {library, legacy.this_id,
            ml290_detail::findCreated(library, legacy.parent_id), ml290_detail::live};
        pending.front() = record;
        items.splice(items.end(), pending);
        return ml290_detail::token(legacy.this_id);
    } catch (const std::bad_alloc&) {
        return 0;
    }
}

inline BOOL MLNavItem_GetInfo(HWND library, NAVITEM *item) {
    if (!item || item->cbSize != sizeof(NAVITEM) || item->mask != NIMF_ITEMID) return FALSE;
    int id = ml290_detail::id(item->hItem);
    if (!MLNavCtrl_FindItemById(library, id)) return FALSE;
    item->id = id;
    return TRUE;
}

inline BOOL MLNavItem_Select(HWND library, HNAVITEM item) {
    return item && SENDMLIPC(library, ML_IPC_SETCURTREEITEM, ml290_detail::id(item));
}

inline BOOL MLNavCtrl_DeleteItem(HWND library, HNAVITEM item) {
    const int id = ml290_detail::id(item);
    ml290_detail::CreatedItem *root = ml290_detail::findCreated(library, id);
    if (!root || root->status != ml290_detail::live) return FALSE;
    root->status = ml290_detail::deleting;
    const BOOL deleted = SENDMLIPC(library, ML_IPC_DELTREEITEM, id) != 0;
    // Synchronous host callbacks can remove an ancestor before the call returns.
    root = ml290_detail::findCreated(library, id);
    if (!root) return deleted;
    root->status = deleted ? ml290_detail::retired : ml290_detail::live;
    if (!deleted) return FALSE;
    std::list<ml290_detail::CreatedItem>& items = ml290_detail::createdItems();
    for (std::list<ml290_detail::CreatedItem>::iterator it = items.begin(); it != items.end(); ++it) {
        if (it->parent && it->parent->status == ml290_detail::retired) it->status = ml290_detail::retired;
    }
    for (std::list<ml290_detail::CreatedItem>::iterator it = items.begin(); it != items.end(); ) {
        if (it->status == ml290_detail::retired) it = items.erase(it);
        else ++it;
    }
    return TRUE;
}

#endif
