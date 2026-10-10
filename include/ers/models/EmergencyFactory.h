#pragma once
#include <memory>
#include <string>
#include "ers/models/Emergency.h"

namespace ers {

std::unique_ptr<Emergency> createEmergency(EmergencyType type, EmergencyId id, NodeId node,
                                           Priority priority, double createdAt,
                                           const std::string& description);

} // namespace ers