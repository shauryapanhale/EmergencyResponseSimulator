#include "ers/models/Vehicle.h"
#include <algorithm>
#include <utility>
#include "ers/common/Strings.h"

namespace ers {

Vehicle::Vehicle(VehicleId id, StationId stationId, NodeId homeNode, double speedKmph,
                 std::set<std::string> equipment)
    : id_(id), stationId_(stationId), homeNode_(homeNode), currentNode_(homeNode),
      speedKmph_(speedKmph), status_(VehicleStatus::Available), equipment_(std::move(equipment)) {
    if (id_ < 0) throw ValidationException("Vehicle id must be non-negative");
    if (stationId_ < 0) throw ValidationException("Vehicle station is invalid");
    if (homeNode_ < 0) throw ValidationException("Vehicle home node is invalid");
    if (speedKmph_ <= 0) throw ValidationException("Vehicle speed must be positive");
}

void Vehicle::setCurrentNode(NodeId n) {
    if (n < 0) throw ValidationException("Node id must be non-negative");
    currentNode_ = n;
}

bool Vehicle::canTransition(VehicleStatus from, VehicleStatus to) {
    using S = VehicleStatus;
    switch (from) {
    case S::Available:    return to == S::EnRoute || to == S::OutOfService;
    case S::EnRoute:      return to == S::OnScene || to == S::OutOfService;
    case S::OnScene:      return to == S::Returning || to == S::Available || to == S::OutOfService;
    case S::Returning:    return to == S::Available || to == S::EnRoute || to == S::OutOfService;
    case S::OutOfService: return to == S::Available;
    }
    return false;
}

void Vehicle::setStatus(VehicleStatus next) {
    if (!canTransition(status_, next))
        throw InvalidTransitionException("Invalid vehicle transition: " + toString(status_) +
                                         " -> " + toString(next));
    status_ = next;
    if (status_ == VehicleStatus::Available) assigned_.reset();
}

void Vehicle::assignIncident(EmergencyId incident) {
    if (status_ != VehicleStatus::Available)
        throw InvalidTransitionException("Vehicle " + std::to_string(id_) + " is not available");
    setStatus(VehicleStatus::EnRoute);
    assigned_ = incident;
}

void Vehicle::markArrived() { setStatus(VehicleStatus::OnScene); }
void Vehicle::markReturning() { setStatus(VehicleStatus::Returning); }

void Vehicle::markBackAtBase() {
    if (status_ != VehicleStatus::OnScene && status_ != VehicleStatus::Returning)
        throw InvalidTransitionException("Vehicle is not on scene or returning");
    setStatus(VehicleStatus::Available);
    currentNode_ = homeNode_;
}

void Vehicle::markOutOfService() { setStatus(VehicleStatus::OutOfService); }
void Vehicle::markInService() { setStatus(VehicleStatus::Available); }

bool Vehicle::needsType(const Emergency& e, VehicleType t) {
    const auto req = e.requiredResources();
    return std::find(req.begin(), req.end(), t) != req.end();
}

} // namespace ers