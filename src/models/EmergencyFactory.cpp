#include "ers/models/EmergencyFactory.h"
#include "ers/models/CrimeEmergency.h"
#include "ers/models/FireEmergency.h"
#include "ers/models/MedicalEmergency.h"
#include "ers/models/RoadAccidentEmergency.h"

namespace ers {

std::unique_ptr<Emergency> createEmergency(EmergencyType type, EmergencyId id, NodeId node,
                                           Priority priority, double createdAt,
                                           const std::string& description) {
    switch (type) {
    case EmergencyType::Medical:
        return std::make_unique<MedicalEmergency>(id, node, priority, createdAt, description);
    case EmergencyType::Fire:
        return std::make_unique<FireEmergency>(id, node, priority, createdAt, description);
    case EmergencyType::RoadAccident:
        return std::make_unique<RoadAccidentEmergency>(id, node, priority, createdAt, description);
    case EmergencyType::Crime:
        return std::make_unique<CrimeEmergency>(id, node, priority, createdAt, description);
    }
    throw InvalidIncidentException("Unknown incident type");
}

} // namespace ers