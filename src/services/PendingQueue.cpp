#include "ers/services/PendingQueue.h"

namespace ers {

void PendingQueue::push(const Emergency& e) {
    int v = nextVersion_++;
    versions_[e.id()] = v;
    heap_.push(Entry{PriorityKey{static_cast<int>(e.priority()), e.createdAt(), e.id()}, v});
}

bool PendingQueue::remove(EmergencyId id) { return versions_.erase(id) > 0; }

bool PendingQueue::contains(EmergencyId id) const { return versions_.count(id) > 0; }

std::optional<EmergencyId> PendingQueue::popNext() {
    while (!heap_.empty()) {
        Entry top = heap_.top();
        heap_.pop();
        auto it = versions_.find(top.key.id);
        if (it != versions_.end() && it->second == top.version) {
            versions_.erase(it);
            return top.key.id;
        }
    }
    return std::nullopt;
}

std::vector<EmergencyId> PendingQueue::snapshot() const {
    Heap copy = heap_;
    std::vector<EmergencyId> order;
    while (!copy.empty()) {
        Entry top = copy.top();
        copy.pop();
        auto it = versions_.find(top.key.id);
        if (it != versions_.end() && it->second == top.version) order.push_back(top.key.id);
    }
    return order;
}

} // namespace ers