#pragma once
#include <string>
#include <vector>
#include "ers/common/Types.h"
#include "ers/services/SimulationEngine.h"

namespace ers {

struct ScenarioIncident {
    double atMinute;
    EmergencyType type;
    NodeId node;
    Priority priority;
    std::string description;
};

struct ScenarioTrafficChange {
    double atMinute;
    RoadId road;
    double factor;
};

struct ScenarioRoadBlock {
    double atMinute;
    RoadId road;
    bool blocked;
};

// A prepared, repeatable set of timed events for the sample city (FR-23).
struct Scenario {
    std::string name;
    std::string description;
    double durationMinutes = 60.0;
    std::vector<ScenarioIncident> incidents;
    std::vector<ScenarioTrafficChange> trafficChanges;
    std::vector<ScenarioRoadBlock> roadBlocks;
};

const std::vector<Scenario>& sampleScenarios();

// Throws ValidationException for an unknown name.
const Scenario& findScenario(const std::string& name);

// Applies each event when simulation time reaches it, stepping until durationMinutes.
void runScenario(SimulationEngine& sim, const Scenario& scenario);

} // namespace ers