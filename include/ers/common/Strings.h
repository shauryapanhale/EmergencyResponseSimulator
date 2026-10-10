#pragma once
#include <string>
#include "ers/common/Types.h"

namespace ers {

inline std::string toString(Priority p) {
    switch (p) {
    case Priority::Critical: return "Critical";
    case Priority::High: return "High";
    case Priority::Medium: return "Medium";
    case Priority::Low: return "Low";
    }
    return "Unknown";
}

inline std::string toString(EmergencyType t) {
    switch (t) {
    case EmergencyType::Medical: return "Medical";
    case EmergencyType::Fire: return "Fire";
    case EmergencyType::RoadAccident: return "Road Accident";
    case EmergencyType::Crime: return "Crime";
    }
    return "Unknown";
}

inline std::string toString(EmergencyStatus s) {
    switch (s) {
    case EmergencyStatus::Pending: return "Pending";
    case EmergencyStatus::Dispatched: return "Dispatched";
    case EmergencyStatus::OnScene: return "On Scene";
    case EmergencyStatus::Resolved: return "Resolved";
    case EmergencyStatus::Unserved: return "Unserved";
    }
    return "Unknown";
}

inline std::string toString(VehicleType t) {
    switch (t) {
    case VehicleType::Ambulance: return "Ambulance";
    case VehicleType::FireTruck: return "Fire Truck";
    case VehicleType::Police: return "Police Vehicle";
    }
    return "Unknown";
}

inline std::string toString(VehicleStatus s) {
    switch (s) {
    case VehicleStatus::Available: return "Available";
    case VehicleStatus::EnRoute: return "En Route";
    case VehicleStatus::OnScene: return "On Scene";
    case VehicleStatus::Returning: return "Returning";
    case VehicleStatus::OutOfService: return "Out of Service";
    }
    return "Unknown";
}

} // namespace ers