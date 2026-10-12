#pragma once
#include <optional>
#include <set>
#include <string>
#include "ers/common/Types.h"
#include "ers/common/Exceptions.h"
#include "ers/models/Emergency.h"

namespace ers {

// Abstract base class for all response vehicles
class Vehicle {
public:
    Vehicle(VehicleId id, StationId stationId, NodeId homeNode, double speedKmph,
            std::set<std::string> equipment = {});
    virtual ~Vehicle() = default;

    Vehicle(const Vehicle&) = delete;
    Vehicle& operator=(const Vehicle&) = delete;

    // Pure virtual: each concrete vehicle defines what it can respond to
    virtual VehicleType type() const = 0;
    virtual bool canRespondTo(const Emergency& e) const = 0;

    VehicleId id() const { return id_; }
    StationId stationId() const { return stationId_; }
    NodeId homeNode() const { return homeNode_; }
    NodeId currentNode() const { return currentNode_; }
    double speedKmph() const { return speedKmph_; }
    static constexpr double kReferenceSpeedKmph = 30.0;  // speed at which road minutes are defined
    double timeFactor() const { return kReferenceSpeedKmph / speedKmph_; }
    VehicleStatus status() const { return status_; }
    bool isAvailable() const { return status_ == VehicleStatus::Available; }
    std::optional<EmergencyId> assignedIncident() const { return assigned_; }
    const std::set<std::string>& equipment() const { return equipment_; }
    bool hasEquipment(const std::string& item) const { return equipment_.count(item) > 0; }

    void setCurrentNode(NodeId n);

    // Validated lifecycle changes (encapsulation)
    void setStatus(VehicleStatus next);
    void assignIncident(EmergencyId incident);  // Available -> EnRoute
    void markArrived();                          // EnRoute -> OnScene
    void markReturning();                        // OnScene -> Returning
    void markBackAtBase();                       // OnScene/Returning -> Available at home node
    void markOutOfService();
    void markInService();                        // OutOfService -> Available

    static bool canTransition(VehicleStatus from, VehicleStatus to);

protected:
    // Helper for subclasses: does the incident need this kind of vehicle?
    static bool needsType(const Emergency& e, VehicleType t);

private:
    VehicleId id_;
    StationId stationId_;
    NodeId homeNode_;
    NodeId currentNode_;
    double speedKmph_;
    VehicleStatus status_;
    std::optional<EmergencyId> assigned_;
    std::set<std::string> equipment_;
};

} // namespace ers