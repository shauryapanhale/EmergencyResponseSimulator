#pragma once
#include "ers/common/Types.h"
#include "ers/common/Exceptions.h"

namespace ers {

class Road {
public:
    // Constructor overloading: simple and fully configured versions
    Road(RoadId id, NodeId from, NodeId to, double distanceKm, double baseMinutes)
        : Road(id, from, to, distanceKm, baseMinutes, 1.0, false, true) {}

    Road(RoadId id, NodeId from, NodeId to, double distanceKm, double baseMinutes,
         double trafficFactor, bool blocked, bool bidirectional)
        : id_(id), from_(from), to_(to), distanceKm_(distanceKm),
          baseMinutes_(baseMinutes), trafficFactor_(trafficFactor),
          blocked_(blocked), bidirectional_(bidirectional) {
        if (id_ < 0) throw ValidationException("Road id must be non-negative");
        if (from_ == to_) throw ValidationException("Road endpoints must differ");
        if (distanceKm_ <= 0) throw ValidationException("Road distance must be positive");
        if (baseMinutes_ <= 0) throw ValidationException("Road base time must be positive");
        if (trafficFactor_ <= 0) throw ValidationException("Traffic factor must be positive");
    }

    RoadId id() const { return id_; }
    NodeId from() const { return from_; }
    NodeId to() const { return to_; }
    double distanceKm() const { return distanceKm_; }
    double baseMinutes() const { return baseMinutes_; }
    double trafficFactor() const { return trafficFactor_; }
    bool isBlocked() const { return blocked_; }
    bool isBidirectional() const { return bidirectional_; }

    // edgeCostMinutes = baseMinutes * trafficFactor
    double currentMinutes() const { return baseMinutes_ * trafficFactor_; }

    void setTrafficFactor(double f) {
        if (f <= 0) throw ValidationException("Traffic factor must be positive");
        trafficFactor_ = f;
    }
    void setBlocked(bool b) { blocked_ = b; }

private:
    RoadId id_;
    NodeId from_;
    NodeId to_;
    double distanceKm_;
    double baseMinutes_;
    double trafficFactor_;
    bool blocked_;
    bool bidirectional_;
};

} // namespace ers