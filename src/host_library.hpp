#pragma once

#include "ml_sdk.hpp"
#include <string>
#include <vector>

namespace tangerine {

// All navigation IDs belong to the host. Legacy IDs never masquerade as the
// handles returned by the newer navigation API.
class HostLibrary {
public:
    void attach(HWND library, HWND player, bool unicodePlaylist);
    bool nativeControls() const { return nativeNavigation_; }
    UINT_PTR insert(std::wstring label, std::wstring invariant, UINT_PTR parent, bool branch);
    UINT_PTR selection() const;
    void select(UINT_PTR id) const;
    bool remove(UINT_PTR id);
    void clearChildren(UINT_PTR parent);
    bool skin(HWND control, UINT type, UINT style) const;
    void divider(HWND control, bool vertical, MLDIVIDERMOVED callback, LPARAM context) const;
    UINT popup(MLSKINNEDPOPUP& popup) const;
    int recordType() const { return unicodePlaylist_ ? ML_TYPE_ITEMRECORDLISTW : ML_TYPE_ITEMRECORDLIST; }
    void sendTracks(itemRecordListW& tracks, bool enqueue, mlDropItemStruct* drop = nullptr) const;

private:
    LRESULT send(UINT command, WPARAM value = 0) const;
    struct LegacyItem { UINT_PTR id, parent; };
    HWND library_ = nullptr;
    HWND player_ = nullptr;
    bool nativeNavigation_ = true, unicodePlaylist_ = true;
    std::vector<LegacyItem> legacyItems_;
};

} // namespace tangerine
