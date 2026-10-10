#pragma once
#include "ers/common/Types.h"

namespace ers {

// Smaller key = more urgent. Order: priority rank, then earlier creation time, then lower id.
struct PriorityKey {
    int rank;
    double createdAt;
    EmergencyId id;

    bool operator<(const PriorityKey& o) const {
        if (rank != o.rank) return rank < o.rank;
        if (createdAt != o.createdAt) return createdAt < o.createdAt;
        return id < o.id;
    }
    bool operator>(const PriorityKey& o) const { return o < *this; }
};

} // namespace ers