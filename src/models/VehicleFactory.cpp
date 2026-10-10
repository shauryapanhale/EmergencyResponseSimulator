#include "ers/models/VehicleFactory.h"
#include "ers/models/Ambulance.h"
#include "ers/models/FireTruck.h"
#include "ers/models/PoliceVehicle.h"

namespace ers {

std::unique_ptr<Vehicle> createVehicle(VehicleType type, VehicleId id, StationId station,
                                       NodeId home, double speedKmph) {
    switch (type) {
    case VehicleType::Ambulance: return std::make_unique<Ambulance>(id, station, home, speedKmph);
    case VehicleType::FireTruck: return std::make_unique<FireTruck>(id, station, home, speedKmph);
    case VehicleType::Police: return std::make_unique<PoliceVehicle>(id, station, home, speedKmph);
    }
    throw ValidationException("Unknown vehicle type");
}

} // namespace ers