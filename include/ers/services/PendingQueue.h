#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>
#include "ers/models/Emergency.h"
#include "ers/models/PriorityKey.h"

namespace ers {

// Pending incidents ordered by PriorityKey. Priority changes use versioned entries:
// pushing an already-queued incident adds a new entry and invalidates the old one.
class PendingQueue {
public:
    void push(const Emergency& e);           // add, or re-prioritize if already queued
    bool remove(EmergencyId id);
    std::optional<EmergencyId> popNext();
    bool contains(EmergencyId id) const;
    std::size_t size() const { return versions_.size(); }
    bool empty() const { return versions_.empty(); }
    std::vector<EmergencyId> snapshot() const;  // current order, most urgent first

private:
    struct Entry {
        PriorityKey key;
        int version;
        bool operator>(const Entry& o) const { return key > o.key; }
    };
    using Heap = std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>>;

    Heap heap_;
    std::unordered_map<EmergencyId, int> versions_;  // id -> latest valid version
    int nextVersion_ = 1;
};

} // namespace ers