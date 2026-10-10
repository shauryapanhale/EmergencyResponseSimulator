#pragma once
#include "ers/models/Emergency.h"

namespace ers {

class RoadAccidentEmergency : public Emergency {
public:
    RoadAccidentEmergency(EmergencyId id, NodeId node, Priority p, double createdAt, std::string desc)
        : RoadAccidentEmergency(id, node, p, createdAt, std::move(desc), 1) {}
    RoadAccidentEmergency(EmergencyId id, NodeId node, Priority p, double createdAt, std::string desc,
                          int casualtiesEstimate)
        : Emergency(id, node, p, createdAt, std::move(desc)), casualties_(casualtiesEstimate) {
        if (casualties_ < 0) throw InvalidIncidentException("Casualty estimate cannot be negative");
    }

    EmergencyType type() const override { return EmergencyType::RoadAccident; }
    std::string typeName() const override { return "Road Accident"; }
    // Ambulance + police; add a fire truck for critical or multi-casualty accidents
    std::vector<VehicleType> requiredResources() const override {
        std::vector<VehicleType> r{VehicleType::Ambulance, VehicleType::Police};
        if (priority() == Priority::Critical || casualties_ >= 3) r.push_back(VehicleType::FireTruck);
        return r;
    }
    int casualtiesEstimate() const { return casualties_; }

private:
    int casualties_;
};

} // namespace ers