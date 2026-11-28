#include "host_library.hpp"
#include "host_player.hpp"
#include "legacy_skin.hpp"
#include <algorithm>
#include <memory>
#include <windowsx.h>

namespace tangerine {
namespace {
struct LegacyDivider { bool vertical; MLDIVIDERMOVED callback; LPARAM context; };
LRESULT CALLBACK dividerProc(HWND window, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data) {
    auto& divider = *reinterpret_cast<LegacyDivider*>(data);
    switch (msg) {
    case WM_SETCURSOR:
        SetCursor(LoadCursorW(nullptr, divider.vertical ? IDC_SIZEWE : IDC_SIZENS));
        return TRUE;
    case WM_LBUTTONDOWN:
        SetCapture(window);
        return 0;
    case WM_MOUSEMOVE:
        if (GetCapture() == window) {
            POINT point{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
            MapWindowPoints(window, GetParent(window), &point, 1);
            divider.callback(window, divider.vertical ? point.x : point.y, divider.context);
            return 0;
        }
        break;
    case WM_LBUTTONUP: case WM_CANCELMODE:
        if (GetCapture() == window) ReleaseCapture();
        return 0;
    case WM_NCDESTROY:
        RemoveWindowSubclass(window, dividerProc, id);
        delete &divider;
        break;
    }
    return DefSubclassProc(window, msg, w, l);
}
}

void HostLibrary::attach(HWND library, HWND player, bool unicodePlaylist) {
    library_ = library;
    player_ = player;
    unicodePlaylist_ = unicodePlaylist;
    const auto navigation = reinterpret_cast<HWND>(send(ML_IPC_NAVCTRL_GETHWND));
    // WACUP implements the native navigation operations but does not expose
    // every optional query, including GETHWND. Retain its established API path.
    const auto version = SendMessageW(player, WM_WA_IPC, 0, IPC_GETVERSION);
    nativeNavigation_ = version >= 0x5050 || (IsWindow(navigation) && (navigation == library || IsChild(library, navigation)));
    legacyItems_.clear();
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
    auto title = toHostText(label.c_str());
    legacyItems_.reserve(legacyItems_.size() + 1);
    mlAddTreeItemStruct item{static_cast<INT_PTR>(parent), title.data(), branch, 0};
    send(ML_IPC_ADDTREEITEM, reinterpret_cast<WPARAM>(&item));
    if (item.this_id <= 0) throw Error("The player could not add a library tree item.");
    legacyItems_.push_back({static_cast<UINT_PTR>(item.this_id), parent});
    return item.this_id;
}

UINT_PTR HostLibrary::selection() const {
    if (!nativeNavigation_) return send(ML_IPC_GETCURTREEITEM);
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
        if (selection() != id) send(ML_IPC_SETCURTREEITEM, id);
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
    // Parents are inserted before descendants. Reserve before asking the host to
    // delete anything, so allocation failure cannot strand the bookkeeping.
    std::vector<UINT_PTR> branch;
    branch.reserve(legacyItems_.size() + 1);
    branch.push_back(id);
    for (const auto& item : legacyItems_)
        if (std::find(branch.begin(), branch.end(), item.parent) != branch.end()) branch.push_back(item.id);
    if (!send(ML_IPC_DELTREEITEM, id)) return false;
    legacyItems_.erase(std::remove_if(legacyItems_.begin(), legacyItems_.end(), [&](const LegacyItem& item) {
        return std::find(branch.begin(), branch.end(), item.id) != branch.end();
    }), legacyItems_.end());
    return true;
}

void HostLibrary::clearChildren(UINT_PTR parent) {
    if (nativeNavigation_) {
        const auto handle = send(ML_IPC_TREEITEM_GETHANDLE, parent);
        while (const auto child = send(ML_IPC_TREEITEM_GETCHILD, handle)) {
            send(ML_IPC_TREEITEM_DELETE, child);
            if (send(ML_IPC_TREEITEM_GETCHILD, handle) == child) throw Error("The player could not refresh the library tree.");
        }
    } else {
        for (;;) {
            const auto child = std::find_if(legacyItems_.begin(), legacyItems_.end(), [&](const LegacyItem& item) { return item.parent == parent; });
            if (child == legacyItems_.end()) break;
            if (!remove(child->id)) throw Error("The player could not refresh the library tree.");
        }
    }
}

bool HostLibrary::skin(HWND control, UINT type, UINT style) const {
    if (nativeNavigation_) {
        MLSKINWINDOW window{control, type, style | SWS_USESKINFONT | SWS_USESKINCOLORS | SWS_USESKINCURSORS};
        return send(ML_IPC_SKINWINDOW, reinterpret_cast<WPARAM>(&window)) != 0;
    }
    return skinLegacyControl(library_, player_, control, type);
}

void HostLibrary::divider(HWND control, bool vertical, MLDIVIDERMOVED callback, LPARAM context) const {
    if (skin(control, SKINNEDWND_TYPE_DIVIDER, vertical ? SWDIV_VERT : 0)) {
        MLSkinnedDivider_SetCallback(control, callback, context);
    } else {
        auto divider = std::make_unique<LegacyDivider>(LegacyDivider{vertical, callback, context});
        if (SetWindowSubclass(control, dividerProc, 1, reinterpret_cast<DWORD_PTR>(divider.get()))) divider.release();
    }
}

UINT HostLibrary::popup(MLSKINNEDPOPUP& popup) const {
    // Zero also means the user cancelled a supported menu. Never retry that
    // cancellation as an unskinned menu.
    if (nativeNavigation_) return static_cast<UINT>(send(ML_IPC_TRACKSKINNEDPOPUPEX, reinterpret_cast<WPARAM>(&popup)));
    return TrackPopupMenuEx(popup.hmenu, popup.fuFlags, popup.x, popup.y, popup.hwnd, nullptr);
}

void HostLibrary::sendTracks(itemRecordListW& tracks, bool enqueue, mlDropItemStruct* drop) const {
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
    if (unicodePlaylist_) { sendList(&tracks); return; }
    struct Tags { std::string file, title, artist, album; };
    std::vector<Tags> tags(tracks.Size);
    std::vector<itemRecord> records(tracks.Size);
    for (int i = 0; i < tracks.Size; ++i) {
        const auto& source = tracks.Items[i];
        auto& tag = tags[i];
        tag = {toHostFilename(source.filename), toHostText(source.title), toHostText(source.artist), toHostText(source.album)};
        auto& target = records[i];
        target.filename = tag.file.data(); target.title = tag.title.data();
        target.artist = tag.artist.data(); target.album = tag.album.data();
        target.length = source.length; target.track = source.track;
    }
    itemRecordList list{records.data(), tracks.Size, tracks.Size};
    sendList(&list);
}

} // namespace tangerine
