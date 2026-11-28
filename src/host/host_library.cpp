#include "host/host_library.hpp"
#include "core/model.hpp"

namespace tangerine {
void HostLibrary::attach(HWND library, HWND player, bool unicodePlaylist) {
    library_ = library;
    unicodePlaylist_ = unicodePlaylist;
    const auto navigation = reinterpret_cast<HWND>(send(ML_IPC_NAVCTRL_GETHWND));
    // WACUP implements the native navigation operations but does not expose
    // every optional query, including GETHWND. Retain its established API path.
    const auto version = SendMessageW(player, WM_WA_IPC, 0, IPC_GETVERSION);
    nativeNavigation_ = version >= 0x5050 || (IsWindow(navigation) && (navigation == library || IsChild(library, navigation)));
    legacy_.attach(library, player);
}

LRESULT HostLibrary::send(UINT command, WPARAM value) const {
    return SendMessageW(library_, WM_ML_IPC, value, command);
}

UINT_PTR HostLibrary::insert(std::wstring label, std::wstring invariant, UINT_PTR parent, bool branch) {
    if (nativeNavigation_) {
        NAVINSERTSTRUCT insert{};
        insert.hParent = parent ? MLNavCtrl_FindItemById(library_, parent) : nullptr;
        insert.hInsertAfter = NCI_LAST;
        auto& item = insert.item;
        item.cbSize = sizeof(item);
        item.mask = NIMF_TEXT | NIMF_TEXTINVARIANT | NIMF_STYLE;
        item.pszText = label.data();
        item.pszInvariant = invariant.data();
        item.styleMask = NIS_HASCHILDREN | NIS_DEFAULTIMAGE;
        item.style = NIS_DEFAULTIMAGE | (branch ? NIS_HASCHILDREN : 0);
        item.hItem = MLNavCtrl_InsertItem(library_, &insert);
        item.mask = NIMF_ITEMID;
        if (!item.hItem || !MLNavItem_GetInfo(library_, &item)) throw Error("The player could not add a library tree item.");
        return item.id;
    }
    return legacy_.insert(label, parent, branch);
}

UINT_PTR HostLibrary::selection() const {
    if (!nativeNavigation_) return legacy_.selection();
    MLTREEITEMINFOW info{};
    info.item.size = sizeof(info.item);
    info.handle = send(ML_IPC_TREEITEM_GETSELECTED);
    info.mask = MLTI_ID;
    if (info.handle) send(ML_IPC_TREEITEM_GETINFOW, reinterpret_cast<WPARAM>(&info));
    return info.item.id;
}

void HostLibrary::select(UINT_PTR id) const {
    if (!id) return;
    if (!nativeNavigation_) {
        legacy_.select(id);
    } else {
        const auto handle = send(ML_IPC_TREEITEM_GETHANDLE, id);
        if (handle && handle != send(ML_IPC_TREEITEM_GETSELECTED)) send(ML_IPC_TREEITEM_SELECT, handle);
    }
}

bool HostLibrary::remove(UINT_PTR id) {
    if (nativeNavigation_) {
        const auto handle = send(ML_IPC_TREEITEM_GETHANDLE, id);
        return handle && send(ML_IPC_TREEITEM_DELETE, handle);
    }
    return legacy_.remove(id);
}

void HostLibrary::clearChildren(UINT_PTR parent) {
    if (nativeNavigation_) {
        const auto handle = send(ML_IPC_TREEITEM_GETHANDLE, parent);
        while (const auto child = send(ML_IPC_TREEITEM_GETCHILD, handle)) {
            send(ML_IPC_TREEITEM_DELETE, child);
            if (send(ML_IPC_TREEITEM_GETCHILD, handle) == child) throw Error("The player could not refresh the library tree.");
        }
    } else legacy_.clearChildren(parent);
}

bool HostLibrary::skin(HWND control, UINT type, UINT style) const {
    if (nativeNavigation_) {
        MLSKINWINDOW window{control, type, style | SWS_USESKINFONT | SWS_USESKINCOLORS | SWS_USESKINCURSORS};
        return send(ML_IPC_SKINWINDOW, reinterpret_cast<WPARAM>(&window)) != 0;
    }
    return legacy_.skin(control, type);
}

void HostLibrary::divider(HWND control, bool vertical, MLDIVIDERMOVED callback, LPARAM context) const {
    if (skin(control, SKINNEDWND_TYPE_DIVIDER, vertical ? SWDIV_VERT : 0)) {
        MLSkinnedDivider_SetCallback(control, callback, context);
    } else legacy_.divider(control, vertical, callback, context);
}

UINT HostLibrary::popup(MLSKINNEDPOPUP& popup) const {
    // Zero also means the user cancelled a supported menu. Never retry that
    // cancellation as an unskinned menu.
    if (nativeNavigation_) return static_cast<UINT>(send(ML_IPC_TRACKSKINNEDPOPUPEX, reinterpret_cast<WPARAM>(&popup)));
    return TrackPopupMenuEx(popup.hmenu, popup.fuFlags, popup.x, popup.y, popup.hwnd, nullptr);
}

void HostLibrary::sendTracks(itemRecordListW& tracks, bool enqueue, mlDropItemStruct* drop) const {
    if (!unicodePlaylist_) { legacy_.sendTracks(tracks, enqueue, drop); return; }
    auto sendList = [&](void* records) {
        if (drop) {
            drop->type = recordType();
            drop->flags = drop->result = 0;
            drop->data = records;
            send(ML_IPC_HANDLEDROP, reinterpret_cast<WPARAM>(drop));
        } else {
            mlSendToWinampStruct request{recordType(), records, enqueue ? 3 : 2};
            send(ML_IPC_SENDTOWINAMP, reinterpret_cast<WPARAM>(&request));
        }
    };
    sendList(&tracks);
}

} // namespace tangerine
