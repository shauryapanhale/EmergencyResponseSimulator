#pragma once
#include "ers/models/Vehicle.h"

namespace ers {

class FireTruck : public Vehicle {
public:
    FireTruck(VehicleId id, StationId station, NodeId home, double speedKmph)
        : Vehicle(id, station, home, speedKmph, {"water tank", "ladder"}) {}
    FireTruck(VehicleId id, StationId station, NodeId home, double speedKmph,
              std::set<std::string> equipment)
        : Vehicle(id, station, home, speedKmph, std::move(equipment)) {}

    VehicleType type() const override { return VehicleType::FireTruck; }
    bool canRespondTo(const Emergency& e) const override {
        return needsType(e, VehicleType::FireTruck);
    }
};

} // namespace ers