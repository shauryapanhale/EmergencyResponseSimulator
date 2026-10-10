#pragma once
#include <memory>
#include "ers/models/Vehicle.h"

namespace ers {

std::unique_ptr<Vehicle> createVehicle(VehicleType type, VehicleId id, StationId station,
                                       NodeId home, double speedKmph);

} // namespace ers