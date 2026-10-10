#pragma once
#include "ers/models/Emergency.h"

namespace ers {

class MedicalEmergency : public Emergency {
public:
    // Constructor overloading
    MedicalEmergency(EmergencyId id, NodeId node, Priority p, double createdAt, std::string desc)
        : MedicalEmergency(id, node, p, createdAt, std::move(desc), "general") {}
    MedicalEmergency(EmergencyId id, NodeId node, Priority p, double createdAt, std::string desc,
                     std::string medicalNeed)
        : Emergency(id, node, p, createdAt, std::move(desc)), medicalNeed_(std::move(medicalNeed)) {}

    EmergencyType type() const override { return EmergencyType::Medical; }
    std::string typeName() const override { return "Medical"; }
    std::vector<VehicleType> requiredResources() const override { return {VehicleType::Ambulance}; }
    const std::string& medicalNeed() const { return medicalNeed_; }

private:
    std::string medicalNeed_;
};

} // namespace ers