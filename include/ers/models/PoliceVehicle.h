#pragma once
#include "ers/models/Vehicle.h"

namespace ers {

class PoliceVehicle : public Vehicle {
public:
    PoliceVehicle(VehicleId id, StationId station, NodeId home, double speedKmph)
        : Vehicle(id, station, home, speedKmph, {"radio", "barricades"}) {}
    PoliceVehicle(VehicleId id, StationId station, NodeId home, double speedKmph,
                  std::set<std::string> equipment)
        : Vehicle(id, station, home, speedKmph, std::move(equipment)) {}

    VehicleType type() const override { return VehicleType::Police; }
    bool canRespondTo(const Emergency& e) const override {
        return needsType(e, VehicleType::Police);
    }
};

} // namespace ers