#include "host/legacy/library.hpp"
#include "host/legacy/player.hpp"
#include "host/legacy/skin.hpp"
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

void LegacyLibrary::attach(HWND library, HWND player) {
    library_ = library;
    player_ = player;
    items_.clear();
}

LRESULT LegacyLibrary::send(UINT command, WPARAM value) const {
    return SendMessageW(library_, WM_ML_IPC, value, command);
}

UINT_PTR LegacyLibrary::insert(const std::wstring& label, UINT_PTR parent, bool branch) {
    auto title = toHostText(label.c_str());
    items_.reserve(items_.size() + 1);
    mlAddTreeItemStruct item{static_cast<INT_PTR>(parent), title.data(), branch, 0};
    send(ML_IPC_ADDTREEITEM, reinterpret_cast<WPARAM>(&item));
    if (item.this_id <= 0) throw Error("The player could not add a library tree item.");
    items_.push_back({static_cast<UINT_PTR>(item.this_id), parent});
    return item.this_id;
}

UINT_PTR LegacyLibrary::selection() const { return send(ML_IPC_GETCURTREEITEM); }

void LegacyLibrary::select(UINT_PTR id) const {
    if (id && selection() != id) send(ML_IPC_SETCURTREEITEM, id);
}

bool LegacyLibrary::remove(UINT_PTR id) {
    // Parents are inserted before descendants. Reserve before asking the host to
    // delete anything, so allocation failure cannot strand the bookkeeping.
    std::vector<UINT_PTR> branch;
    branch.reserve(items_.size() + 1);
    branch.push_back(id);
    for (const auto& item : items_)
        if (std::find(branch.begin(), branch.end(), item.parent) != branch.end()) branch.push_back(item.id);
    if (!send(ML_IPC_DELTREEITEM, id)) return false;
    items_.erase(std::remove_if(items_.begin(), items_.end(), [&](const Item& item) {
        return std::find(branch.begin(), branch.end(), item.id) != branch.end();
    }), items_.end());
    return true;
}

void LegacyLibrary::clearChildren(UINT_PTR parent) {
    for (;;) {
        const auto child = std::find_if(items_.begin(), items_.end(), [&](const Item& item) { return item.parent == parent; });
        if (child == items_.end()) break;
        if (!remove(child->id)) throw Error("The player could not refresh the library tree.");
    }
}

bool LegacyLibrary::skin(HWND control, UINT type) const {
    return skinLegacyControl(library_, player_, control, type);
}

void LegacyLibrary::divider(HWND control, bool vertical, MLDIVIDERMOVED callback, LPARAM context) const {
    auto divider = std::make_unique<LegacyDivider>(LegacyDivider{vertical, callback, context});
    if (SetWindowSubclass(control, dividerProc, 1, reinterpret_cast<DWORD_PTR>(divider.get()))) divider.release();
}

void LegacyLibrary::sendTracks(itemRecordListW& tracks, bool enqueue, mlDropItemStruct* drop) const {
    auto sendList = [&](void* records) {
        if (drop) {
            drop->type = ML_TYPE_ITEMRECORDLIST;
            drop->flags = drop->result = 0;
            drop->data = records;
            send(ML_IPC_HANDLEDROP, reinterpret_cast<WPARAM>(drop));
        } else {
            mlSendToWinampStruct request{ML_TYPE_ITEMRECORDLIST, records, enqueue ? 3 : 2};
            send(ML_IPC_SENDTOWINAMP, reinterpret_cast<WPARAM>(&request));
        }
    };
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
