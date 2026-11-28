#pragma once
#include "host/ml_sdk.hpp"
#include <string>
#include <vector>

namespace tangerine {
class LegacyLibrary {
public:
    void attach(HWND library, HWND player);
    UINT_PTR insert(const std::wstring& label, UINT_PTR parent, bool branch);
    UINT_PTR selection() const;
    void select(UINT_PTR id) const;
    bool remove(UINT_PTR id);
    void clearChildren(UINT_PTR parent);
    bool skin(HWND control, UINT type) const;
    void divider(HWND control, bool vertical, MLDIVIDERMOVED callback, LPARAM context) const;
    void sendTracks(itemRecordListW& tracks, bool enqueue, mlDropItemStruct* drop) const;
private:
    LRESULT send(UINT command, WPARAM value = 0) const;
    struct Item { UINT_PTR id, parent; };
    HWND library_ = nullptr, player_ = nullptr;
    std::vector<Item> items_;
};
}
