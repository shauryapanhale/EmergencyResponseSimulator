#pragma once
#include "ers/models/Emergency.h"

namespace ers {

class FireEmergency : public Emergency {
public:
    FireEmergency(EmergencyId id, NodeId node, Priority p, double createdAt, std::string desc)
        : FireEmergency(id, node, p, createdAt, std::move(desc), "structure") {}
    FireEmergency(EmergencyId id, NodeId node, Priority p, double createdAt, std::string desc,
                  std::string fireClass)
        : Emergency(id, node, p, createdAt, std::move(desc)), fireClass_(std::move(fireClass)) {}

    EmergencyType type() const override { return EmergencyType::Fire; }
    std::string typeName() const override { return "Fire"; }
    // A critical fire also requests ambulance support
    std::vector<VehicleType> requiredResources() const override {
        std::vector<VehicleType> r{VehicleType::FireTruck};
        if (priority() == Priority::Critical) r.push_back(VehicleType::Ambulance);
        return r;
    }
    const std::string& fireClass() const { return fireClass_; }

private:
    std::string fireClass_;
};

} // namespace ers