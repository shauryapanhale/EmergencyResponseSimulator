#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "ers/repositories/SampleCity.h"
#include "ers/services/Scenario.h"
#include "ers/services/SimulationEngine.h"

namespace {
int failures = 0;
}

#define EXPECT(cond)                                                          \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "FAIL line " << __LINE__ << ": " #cond << "\n";      \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

static bool close(double a, double b) { return std::fabs(a - b) < 1e-9; }

static ers::StatsSnapshot runBatch(const std::string& name) {
    ers::CityData city = ers::buildSampleCity();
    ers::SimulationConfig cfg;
    cfg.autoResolveMinutes = 3.0;
    ers::SimulationEngine sim(city.map, ers::buildSampleFleet(), cfg);
    ers::runScenario(sim, ers::findScenario(name));
    return sim.stats();
}

static ers::StatsSnapshot runStepwise(const std::string& name) {
    ers::CityData city = ers::buildSampleCity();
    ers::SimulationConfig cfg;
    cfg.autoResolveMinutes = 3.0;
    ers::SimulationEngine sim(city.map, ers::buildSampleFleet(), cfg);
    const ers::Scenario& sc = ers::findScenario(name);
    ers::ScenarioPlayer player(sim, sc);
    EXPECT(!player.finished());
    int guard = 0;
    while (!player.finished() && guard++ < 100000) player.step();
    EXPECT(player.finished());
    EXPECT(sim.time() >= sc.durationMinutes - 1e-9);
    return sim.stats();
}

int main() {
    for (const ers::Scenario& sc : ers::sampleScenarios()) {
        const ers::StatsSnapshot a = runBatch(sc.name);
        const ers::StatsSnapshot b = runStepwise(sc.name);
        std::cout << sc.name << ": created=" << b.created << " resolved=" << b.resolved
                  << " responses=" << b.responseCount << "\n";
        EXPECT(a.created == b.created);
        EXPECT(a.resolved == b.resolved);
        EXPECT(a.resolvedWithUnmet == b.resolvedWithUnmet);
        EXPECT(a.responseCount == b.responseCount);
        EXPECT(close(a.avgResponse, b.avgResponse));
        EXPECT(close(a.avgWaiting, b.avgWaiting));
        EXPECT(close(a.avgResolution, b.avgResolution));
    }

    {
        ers::CityData city = ers::buildSampleCity();
        ers::SimulationEngine sim(city.map, ers::buildSampleFleet());
        ers::ScenarioPlayer player(sim, ers::findScenario("simultaneous-incidents"));
        EXPECT(sim.incidentIds().empty());
        player.applyDueEvents();
        EXPECT(sim.incidentIds().size() == 6);
        player.applyDueEvents();
        EXPECT(sim.incidentIds().size() == 6);
        player.step();
        EXPECT(sim.incidentIds().size() == 6);
        EXPECT(close(sim.time(), 1.0));
    }

    if (failures == 0) std::cout << "ScenarioPlayerTests: all checks passed\n";
    return failures == 0 ? 0 : 1;
}