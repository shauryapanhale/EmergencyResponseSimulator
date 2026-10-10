#pragma once
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "ers/common/Types.h"
#include "ers/common/Exceptions.h"

namespace ers {

enum class StationType { AmbulanceBase, FireStation, PoliceStation, Shared };

// Aggregation: a station references its vehicles by id, it does not own them
class Station {
public:
    Station(StationId id, std::string name, NodeId nodeId, StationType type)
        : id_(id), name_(std::move(name)), nodeId_(nodeId), type_(type) {
        if (id_ < 0) throw ValidationException("Station id must be non-negative");
        if (nodeId_ < 0) throw ValidationException("Station location is invalid");
        if (name_.empty()) throw ValidationException("Station name must not be empty");
    }

    StationId id() const { return id_; }
    const std::string& name() const { return name_; }
    NodeId nodeId() const { return nodeId_; }
    StationType type() const { return type_; }
    const std::vector<VehicleId>& vehicleIds() const { return vehicles_; }

    void addVehicle(VehicleId v) {
        if (std::find(vehicles_.begin(), vehicles_.end(), v) != vehicles_.end())
            throw ValidationException("Vehicle already assigned to this station");
        vehicles_.push_back(v);
    }
    bool removeVehicle(VehicleId v) {
        auto it = std::find(vehicles_.begin(), vehicles_.end(), v);
        if (it == vehicles_.end()) return false;
        vehicles_.erase(it);
        return true;
    }

private:
    StationId id_;
    std::string name_;
    NodeId nodeId_;
    StationType type_;
    std::vector<VehicleId> vehicles_;
};

} // namespace ers