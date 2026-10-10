#pragma once
#include "ers/models/Vehicle.h"

namespace ers {

class Ambulance : public Vehicle {
public:
    // Constructor overloading: default sample equipment vs fully configured
    Ambulance(VehicleId id, StationId station, NodeId home, double speedKmph)
        : Vehicle(id, station, home, speedKmph, {"stretcher", "defibrillator"}) {}
    Ambulance(VehicleId id, StationId station, NodeId home, double speedKmph,
              std::set<std::string> equipment)
        : Vehicle(id, station, home, speedKmph, std::move(equipment)) {}

    VehicleType type() const override { return VehicleType::Ambulance; }
    bool canRespondTo(const Emergency& e) const override {
        return needsType(e, VehicleType::Ambulance);
    }
};

} // namespace ers