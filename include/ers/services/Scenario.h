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

// Step-by-step version of runScenario for the GUI: applies each event when simulation
// time reaches it and advances one tick per step(). Same event order as runScenario.
class ScenarioPlayer {
public:
    ScenarioPlayer(SimulationEngine& sim, const Scenario& scenario);
    bool finished() const;        // simulation time has reached the scenario duration
    void applyDueEvents();        // apply events due at the current time (safe to repeat)
    void step();                  // applyDueEvents(), then one engine tick; no-op if finished
    const Scenario& scenario() const { return scenario_; }

private:
    SimulationEngine& sim_;
    const Scenario& scenario_;
    std::vector<bool> incidentDone_;
    std::vector<bool> trafficDone_;
    std::vector<bool> blockDone_;
};

} // namespace ers