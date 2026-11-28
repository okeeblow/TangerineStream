#pragma once

#include "host/ml_sdk.hpp"
#include "host/legacy/library.hpp"
#include <string>

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
    HWND library_ = nullptr;
    bool nativeNavigation_ = true, unicodePlaylist_ = true;
    LegacyLibrary legacy_;
};

} // namespace tangerine
