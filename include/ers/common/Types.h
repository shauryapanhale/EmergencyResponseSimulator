#pragma once
namespace ers {

using NodeId = int;
using RoadId = int;
using EmergencyId = int;
using VehicleId = int;
using StationId = int;

enum class Priority { Critical = 0, High = 1, Medium = 2, Low = 3 };
enum class EmergencyType { Medical, Fire, RoadAccident, Crime };
enum class VehicleType { Ambulance, FireTruck, Police };
enum class VehicleStatus { Available, EnRoute, OnScene, Returning, OutOfService };
enum class EmergencyStatus { Pending, Dispatched, OnScene, Resolved, Unserved };

} // namespace ers