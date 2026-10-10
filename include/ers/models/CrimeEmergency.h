#pragma once
#include "ers/models/Emergency.h"

namespace ers {

class CrimeEmergency : public Emergency {
public:
    CrimeEmergency(EmergencyId id, NodeId node, Priority p, double createdAt, std::string desc)
        : CrimeEmergency(id, node, p, createdAt, std::move(desc), "general") {}
    CrimeEmergency(EmergencyId id, NodeId node, Priority p, double createdAt, std::string desc,
                   std::string category)
        : Emergency(id, node, p, createdAt, std::move(desc)), category_(std::move(category)) {}

    EmergencyType type() const override { return EmergencyType::Crime; }
    std::string typeName() const override { return "Crime"; }
    std::vector<VehicleType> requiredResources() const override { return {VehicleType::Police}; }
    const std::string& category() const { return category_; }

private:
    std::string category_;
};

} // namespace ers