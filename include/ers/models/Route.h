#pragma once
#include <utility>
#include <vector>
#include "ers/common/Types.h"

namespace ers {

// Value type: copy and move are explicit so you can show them in the viva
class Route {
public:
    Route() = default;
    Route(std::vector<NodeId> nodes, std::vector<RoadId> roads, double totalMinutes)
        : nodes_(std::move(nodes)), roads_(std::move(roads)), totalMinutes_(totalMinutes) {}

    Route(const Route&) = default;
    Route& operator=(const Route&) = default;
    Route(Route&&) noexcept = default;
    Route& operator=(Route&&) noexcept = default;

    const std::vector<NodeId>& nodes() const { return nodes_; }
    const std::vector<RoadId>& roadIds() const { return roads_; }
    double totalMinutes() const { return totalMinutes_; }
    bool empty() const { return nodes_.empty(); }

private:
    std::vector<NodeId> nodes_;
    std::vector<RoadId> roads_;
    double totalMinutes_ = 0.0;
};

} // namespace ers