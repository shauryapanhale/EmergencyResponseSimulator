#include "ers/services/Scenario.h"
#include "ers/common/Exceptions.h"

namespace ers {

namespace {

std::vector<Scenario> makeScenarios() {
    std::vector<Scenario> all;

    {
        Scenario s;
        s.name = "normal-traffic";
        s.description = "Four incidents spread over an hour with normal road conditions.";
        s.durationMinutes = 60.0;
        s.incidents = {
            {0.0,  EmergencyType::Medical,      7,  Priority::High,   "fall in Old Town"},
            {5.0,  EmergencyType::Fire,         9,  Priority::High,   "fire at Tech Park"},
            {12.0, EmergencyType::Crime,        10, Priority::Medium, "theft at College Campus"},
            {20.0, EmergencyType::RoadAccident, 8,  Priority::High,   "collision at Riverside"},
        };
        all.push_back(std::move(s));
    }

    {
        Scenario s;
        s.name = "rush-hour";
        s.description = "Heavy traffic (1.5x) on main roads and severe traffic (2.0x) on two links.";
        s.durationMinutes = 90.0;
        s.trafficChanges = {
            {0.0, 1, 1.5}, {0.0, 3, 1.5}, {0.0, 7, 1.5}, {0.0, 8, 1.5},
            {0.0, 11, 1.5}, {0.0, 12, 2.0}, {0.0, 18, 2.0},
        };
        s.incidents = {
            {0.0,  EmergencyType::Medical,      7,  Priority::High,     "fall in Old Town"},
            {5.0,  EmergencyType::Fire,         9,  Priority::Critical, "fire at Tech Park"},
            {12.0, EmergencyType::Crime,        10, Priority::Medium,   "theft at College Campus"},
            {20.0, EmergencyType::RoadAccident, 8,  Priority::High,     "collision at Riverside"},
            {25.0, EmergencyType::Medical,      6,  Priority::Medium,   "faint at Railway Station"},
        };
        all.push_back(std::move(s));
    }

    {
        Scenario s;
        s.name = "blocked-road";
        s.description = "Roads 9 and 18 are closed so police must take a longer route to Old Town; "
                        "road 9 reopens at minute 30.";
        s.durationMinutes = 60.0;
        s.roadBlocks = {
            {0.0, 9, true}, {0.0, 18, true}, {30.0, 9, false},
        };
        s.incidents = {
            {0.0,  EmergencyType::Crime,   7,  Priority::High,     "disturbance in Old Town"},
            {2.0,  EmergencyType::Medical, 7,  Priority::High,     "injury in Old Town"},
            {10.0, EmergencyType::Fire,    10, Priority::Critical, "fire at College Campus"},
        };
        all.push_back(std::move(s));
    }

    {
        Scenario s;
        s.name = "simultaneous-incidents";
        s.description = "Six incidents at once; more demand than the fleet can serve immediately.";
        s.durationMinutes = 90.0;
        s.incidents = {
            {0.0, EmergencyType::Medical,      7,  Priority::Critical, "cardiac arrest in Old Town"},
            {0.0, EmergencyType::Medical,      9,  Priority::High,     "injury at Tech Park"},
            {0.0, EmergencyType::Medical,      10, Priority::Medium,   "fall at College Campus"},
            {0.0, EmergencyType::Fire,         8,  Priority::Critical, "fire at Riverside"},
            {0.0, EmergencyType::Crime,        6,  Priority::High,     "robbery at Railway Station"},
            {0.0, EmergencyType::RoadAccident, 12, Priority::Critical, "pile-up near Stadium"},
        };
        all.push_back(std::move(s));
    }

    return all;
}

} // namespace

const std::vector<Scenario>& sampleScenarios() {
    static const std::vector<Scenario> scenarios = makeScenarios();
    return scenarios;
}

const Scenario& findScenario(const std::string& name) {
    for (const Scenario& s : sampleScenarios()) {
        if (s.name == name) return s;
    }
    throw ValidationException("Unknown scenario: " + name);
}

void runScenario(SimulationEngine& sim, const Scenario& scenario) {
    std::vector<bool> incidentDone(scenario.incidents.size(), false);
    std::vector<bool> trafficDone(scenario.trafficChanges.size(), false);
    std::vector<bool> blockDone(scenario.roadBlocks.size(), false);
    const double eps = 1e-9;

    for (int guard = 0; sim.time() < scenario.durationMinutes - eps && guard < 100000; ++guard) {
        for (std::size_t i = 0; i < scenario.trafficChanges.size(); ++i) {
            const auto& e = scenario.trafficChanges[i];
            if (!trafficDone[i] && e.atMinute <= sim.time() + eps) {
                sim.setTraffic(e.road, e.factor);
                trafficDone[i] = true;
            }
        }
        for (std::size_t i = 0; i < scenario.roadBlocks.size(); ++i) {
            const auto& e = scenario.roadBlocks[i];
            if (!blockDone[i] && e.atMinute <= sim.time() + eps) {
                sim.setBlocked(e.road, e.blocked);
                blockDone[i] = true;
            }
        }
        for (std::size_t i = 0; i < scenario.incidents.size(); ++i) {
            const auto& e = scenario.incidents[i];
            if (!incidentDone[i] && e.atMinute <= sim.time() + eps) {
                sim.reportIncident(e.type, e.node, e.priority, e.description);
                incidentDone[i] = true;
            }
        }
        sim.step();
    }
}

ScenarioPlayer::ScenarioPlayer(SimulationEngine& sim, const Scenario& scenario)
    : sim_(sim),
      scenario_(scenario),
      incidentDone_(scenario.incidents.size(), false),
      trafficDone_(scenario.trafficChanges.size(), false),
      blockDone_(scenario.roadBlocks.size(), false) {}

bool ScenarioPlayer::finished() const {
    return sim_.time() >= scenario_.durationMinutes - 1e-9;
}

void ScenarioPlayer::applyDueEvents() {
    const double eps = 1e-9;
    for (std::size_t i = 0; i < scenario_.trafficChanges.size(); ++i) {
        const auto& e = scenario_.trafficChanges[i];
        if (!trafficDone_[i] && e.atMinute <= sim_.time() + eps) {
            sim_.setTraffic(e.road, e.factor);
            trafficDone_[i] = true;
        }
    }
    for (std::size_t i = 0; i < scenario_.roadBlocks.size(); ++i) {
        const auto& e = scenario_.roadBlocks[i];
        if (!blockDone_[i] && e.atMinute <= sim_.time() + eps) {
            sim_.setBlocked(e.road, e.blocked);
            blockDone_[i] = true;
        }
    }
    for (std::size_t i = 0; i < scenario_.incidents.size(); ++i) {
        const auto& e = scenario_.incidents[i];
        if (!incidentDone_[i] && e.atMinute <= sim_.time() + eps) {
            sim_.reportIncident(e.type, e.node, e.priority, e.description);
            incidentDone_[i] = true;
        }
    }
}

void ScenarioPlayer::step() {
    if (finished()) return;
    applyDueEvents();
    sim_.step();
}

} // namespace ers